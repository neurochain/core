#ifndef NEURO_SRC_LEDGERMONGODB_HPP
#define NEURO_SRC_LEDGERMONGODB_HPP

#include <mutex>

#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>
#include "common/types.hpp"
#include "config.pb.h"
#include "ledger/Ledger.hpp"
#include "ledger/mongo.hpp"
#include "messages.pb.h"
#include "messages/config/Database.hpp"

namespace neuro {
namespace tooling {
class Simulator;
}

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
  mutable mongocxx::collection _pii;
  mutable mongocxx::collection _assemblies;

  mutable std::mutex _ledger_mutex;

  messages::TaggedBlock _main_branch_tip;

  mongocxx::options::find remove_OID() const;

  mongocxx::options::find projection(const std::string &field) const;

  mongocxx::options::find projection(const std::string &field0,
                                     const std::string &field1) const;

  bool init_block0(const messages::config::Database &config);

  bool load_block0(const messages::config::Database &config,
                   messages::Block *block0);

  void init_database(const messages::Block &block0);

  bool is_ancestor(const messages::TaggedBlock &ancestor,
                   const messages::TaggedBlock &block) const;

  bool is_main_branch(
      const messages::TaggedTransaction &tagged_transaction) const;

  int fill_block_transactions(messages::Block *block) const;

  bool unsafe_get_block(const messages::Hash &id,
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

  bool unsafe_get_block_by_previd(const messages::Hash &previd,
                                  messages::Block *block,
                                  bool include_transactions = true) const;

  messages::BlockHeight unsafe_height() const;

  bool unsafe_get_assembly(const messages::Hash &assembly_id,
                           messages::Assembly *assembly) const;

  void create_indexes();

  void create_first_assemblies(const std::vector<messages::Address> &addresses);

  bool cleanup_transaction_pool(const messages::Hash &block_id);

 public:
  LedgerMongodb(const std::string &url, const std::string &db_name);
  LedgerMongodb(const messages::config::Database &config);

  ~LedgerMongodb() {}

  void remove_all();

  messages::TaggedBlock get_main_branch_tip() const;

  bool set_main_branch_tip();

  messages::BlockHeight height() const;

  bool get_block_header(const messages::BlockID &id,
                        messages::BlockHeader *header) const;

  bool get_last_block_header(messages::BlockHeader *block_header) const;

  bool get_last_block(messages::TaggedBlock *tagged_block,
                      bool include_transactions = true) const;

  bool get_block(const messages::BlockID &id,
                 messages::TaggedBlock *tagged_block,
                 bool include_transactions = true) const;

  bool get_block(const messages::BlockID &id, messages::Block *block,
                 bool include_transactions = true) const;

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

  bool delete_block_and_children(const messages::Hash &id);

  bool get_transaction(const messages::Hash &id,
                       messages::Transaction *transaction) const;

  bool get_transaction(const messages::Hash &id,
                       messages::Transaction *transaction,
                       messages::BlockHeight *blockheight) const;

  bool get_transaction(const messages::Hash &id,
                       messages::TaggedTransaction *tagged_transaction,
                       const messages::TaggedBlock &tip,
                       bool include_transaction_pool) const;

  std::size_t total_nb_transactions() const;

  std::size_t total_nb_blocks() const;

  bool for_each(const Filter &filter, const messages::TaggedBlock &tip,
                bool include_transaction_pool, Functor functor) const;

  bool for_each(const Filter &filter, Functor functor) const;

  int new_branch_id();

  bool add_transaction(const messages::TaggedTransaction &tagged_transaction);

  bool add_to_transaction_pool(const messages::Transaction &transaction);

  bool delete_transaction(const messages::Hash &id);

  std::size_t get_transaction_pool(messages::Block *block);

  bool get_unverified_blocks(
      std::vector<messages::TaggedBlock> *tagged_blocks) const;

  bool set_block_verified(const messages::Hash &id,
                          const messages::BlockScore &score,
                          const messages::Hash previous_assembly_id);

  bool update_main_branch();

  messages::BranchPath fork_from(const messages::BranchPath &branch_path) const;

  messages::BranchPath first_child(
      const messages::BranchPath &branch_path) const;

  bool get_pii(const messages::Address &address,
               const messages::Hash &assembly_id, Double *score) const;

  bool save_pii(const messages::Address &address,
                const messages::Hash &assembly_id, const Double &score);

  void empty_database();

  bool get_assembly(const messages::Hash &assembly_id,
                    messages::Assembly *assembly) const;

  bool add_assembly(const messages::TaggedBlock &tagged_block,
                    const messages::BlockHeight &first_height,
                    const messages::BlockHeight &last_height);

  bool get_pii(const messages::Address &address,
               const messages::Hash &assembly_id, messages::Pii *pii) const;

  bool set_pii(const messages::Pii &pii);

  bool set_previous_assembly_id(const messages::Hash &block_id,
                                const messages::Hash &previous_assembly_id);

  bool get_next_assembly(const messages::Hash &assembly_id,
                         messages::Assembly *assembly) const;

  bool get_assemblies_to_compute(
      std::vector<messages::Assembly> *assemblies) const;

  bool get_block_writer(const messages::Hash &assembly_id, int32_t address_rank,
                        messages::Address *address) const;

  bool set_nb_addresses(const messages::Hash &assembly_id,
                        int32_t nb_addresses);

  bool set_seed(const messages::Hash &assembly_id, int32_t seed);

  bool set_finished_computation(const messages::Hash &assembly_id);

  friend class neuro::ledger::tests::LedgerMongodb;
  friend class neuro::tooling::Simulator;
};

}  // namespace ledger
}  // namespace neuro

#endif /* NEURO_SRC_LEDGERMONGODB_HPP */
