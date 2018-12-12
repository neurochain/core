#ifndef NEURO_SRC_LEDGERMONGODB_HPP
#define NEURO_SRC_LEDGERMONGODB_HPP

#include <mutex>

#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>
#include "config.pb.h"
#include "ledger/Ledger.hpp"
#include "ledger/mongo.hpp"
#include "messages.pb.h"
#include "messages/config/Database.hpp"

namespace neuro {
namespace ledger {
namespace tests {
class LedgerMongodb;
}

class LedgerMongodb : public Ledger {
 private:
  static mongocxx::instance _instance;
  mutable mongocxx::uri _uri;
  mutable mongocxx::client _client;
  mutable mongocxx::database _db;
  mutable mongocxx::collection _blocks;
  mutable mongocxx::collection _transactions;

  mutable std::mutex _ledger_mutex;

  mongocxx::options::find remove_OID() const;

  mongocxx::options::find projection(const std::string &field) const;

  mongocxx::options::find projection(const std::string &field0,
                                     const std::string &field1) const;

  bool init_block0(const messages::config::Database &config);

  bool is_ancestor(const messages::TaggedBlock &ancestor,
                   const messages::TaggedBlock &block) const;

  bool is_main_branch(
      const messages::TaggedTransaction &tagged_transaction) const;

  int fill_block_transactions(messages::Block *block) const;

  bool unsafe_get_block(const messages::BlockID &id,
                        messages::TaggedBlock *tagged_block,
                        bool include_transactions = true) const;

  bool unsafe_get_block(const messages::BlockHeight height,
                        messages::Block *block,
                        bool include_transactions = true) const;

  bool unsafe_get_block(const messages::BlockHeight height,
                        messages::TaggedBlock *tagged_block,
                        bool include_transaction = true) const;

  bool unsafe_get_blocks_by_previd(
      const messages::BlockID &previd,
      std::vector<messages::TaggedBlock> *tagged_blocks,
      bool include_transactions = true) const;

  messages::BranchID new_branch_id() const;

  bool set_branch_path(const messages::BlockHeader &block_header,
                       const messages::BranchPath &branch_path);

  bool set_branch_path(const messages::BlockHeader &block_header);

  bool unsafe_insert_block(messages::TaggedBlock *tagged_block);

  messages::BranchPath unsafe_fork_from(
      const messages::BranchPath &branch_path) const;

  messages::BranchPath unsafe_first_child(
      const messages::BranchPath &branch_path) const;

  bool update_branch_tag(const messages::Hash &id,
                         const messages::Branch &branch);

  bool unsafe_get_block_by_previd(const messages::BlockID &previd,
                                  messages::Block *block,
                                  bool include_transactions = true) const;

 public:
  LedgerMongodb(const std::string &url, const std::string &db_name);
  LedgerMongodb(const messages::config::Database &config);

  ~LedgerMongodb() {}

  void remove_all();

  messages::BlockHeight height() const;

  bool get_block_header(const messages::BlockID &id,
                        messages::BlockHeader *header) const;

  bool get_last_block_header(messages::BlockHeader *block_header) const;

  bool get_block(const messages::BlockID &id,
                 messages::TaggedBlock *tagged_block) const;

  bool get_block(const messages::BlockID &id, messages::Block *block) const;

  bool get_block_by_previd(const messages::BlockID &previd,
                           messages::Block *block,
                           bool include_transactions = true) const;

  bool get_blocks_by_previd(const messages::BlockID &previd,
                            std::vector<messages::TaggedBlock> *tagged_blocks,
                            bool include_transactions = true) const;

  bool get_block(const messages::BlockHeight height, messages::Block *block,
                 bool include_transactions = true) const;

  bool get_block(const messages::BlockHeight height,
                 messages::TaggedBlock *tagged_block,
                 bool include_transaction = true) const;

  bool insert_block(messages::TaggedBlock *tagged_block);

  bool insert_block(messages::Block *block);

  bool delete_block(const messages::Hash &id);

  bool get_transaction(const messages::Hash &id,
                       messages::Transaction *transaction) const;

  bool get_transaction(const messages::Hash &id,
                       messages::Transaction *transaction,
                       messages::BlockHeight *blockheight) const;

  std::size_t total_nb_transactions() const;

  std::size_t total_nb_blocks() const;

  bool for_each(const Filter &filter, Functor functor) const;

  bool add_transaction(const messages::TaggedTransaction &tagged_transaction);

  bool delete_transaction(const messages::Hash &id);

  std::size_t get_transaction_pool(messages::Block *block);

  bool get_unscored_forks(std::vector<messages::TaggedBlock> *tagged_blocks,
                          bool include_transactions = true) const;

  bool set_block_score(const messages::Hash &id, messages::BlockScore score);

  bool update_main_branch(messages::TaggedBlock *main_branch_tip);

  messages::BranchPath fork_from(const messages::BranchPath &branch_path) const;

  messages::BranchPath first_child(
      const messages::BranchPath &branch_path) const;

  friend class neuro::ledger::tests::LedgerMongodb;
};

}  // namespace ledger
}  // namespace neuro

#endif /* NEURO_SRC_LEDGERMONGODB_HPP */
