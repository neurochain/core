#ifndef NEURO_SRC_LEDGERMONGODB_HPP
#define NEURO_SRC_LEDGERMONGODB_HPP

#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>
#include <cstdint>
#include <mutex>

#include "bsoncxx/document/view_or_value.hpp"
#include "common.pb.h"
#include "common/types.hpp"
#include "config.pb.h"
#include "consensus.pb.h"
#include "ledger/Ledger.hpp"
#include "ledger/mongo.hpp"
#include "messages.pb.h"
#include "messages/Message.hpp"
#include "messages/config/Database.hpp"
#include "mongocxx/client.hpp"
#include "mongocxx/collection.hpp"
#include "mongocxx/cursor.hpp"
#include "mongocxx/database.hpp"
#include "mongocxx/instance.hpp"
#include "mongocxx/options/find.hpp"
#include "mongocxx/uri.hpp"

namespace neuro {
namespace tooling {
class RealtimeSimulator;
class Simulator;
}  // namespace tooling

namespace ledger {
namespace tests {
class LedgerMongodb;
}

class LedgerMongodb : public Ledger {
  struct BalanceChange {
    messages::NCCValue positive = 0;
    messages::NCCValue negative = 0;
  };

 private:
  static mongocxx::instance _instance;
  mutable mongocxx::uri _uri;
  mutable mongocxx::client _client;
  mutable mongocxx::database _db;
  mutable mongocxx::collection _blocks;
  mutable mongocxx::collection _transactions;
  mutable mongocxx::collection _pii;
  mutable mongocxx::collection _integrity;
  mutable mongocxx::collection _assemblies;
  mutable mongocxx::collection _double_mining;

  static std::recursive_mutex _ledger_mutex;

  messages::TaggedBlock _main_branch_tip;

  static mongocxx::options::find remove_OID();

  static mongocxx::options::find remove_balances();

  mongocxx::options::find projection(const std::string &field) const;

  mongocxx::options::find projection(const std::string &field0,
                                     const std::string &field1) const;

  bool init_block0(const messages::config::Database &config);

  bool load_block0(const messages::config::Database &config,
                   messages::Block *block0);

  bool is_main_branch(
      const messages::TaggedTransaction &tagged_transaction) const;

  int fill_block_transactions(messages::Block *block) const;

  bool get_block(const messages::BlockHeight height,
                 const messages::BranchPath &branch_path,
                 messages::TaggedBlock *tagged_block,
                 bool include_transactions = true) const;

  messages::TaggedBlocks get_blocks(mongocxx::cursor &cursor,
                                    bool include_transactions) const;

  messages::BranchID new_branch_id() const;

  bool set_branch_path(const messages::BlockHeader &block_header,
                       const messages::BranchPath &branch_path);

  bool set_branch_path(const messages::BlockHeader &block_header);

  bool update_branch_tag(const messages::BlockID &id,
                         const messages::Branch &branch);

  void create_indexes();

  void create_first_assemblies(const std::vector<messages::_KeyPub> &key_pubs);

  bool cleanup_transaction_pool(const messages::BlockID &block_id);

  std::size_t cleanup_transaction_pool();

  void add_transaction_to_balances(
      std::unordered_map<messages::_KeyPub, BalanceChange> *balance_changes,
      const messages::Transaction &transaction);

  mongocxx::cursor find(
      mongocxx::collection &collection, bsoncxx::document::view_or_value filter,
      const mongocxx::options::find &options = remove_OID()) const;

  Double compute_new_balance(messages::Balance *balance,
                             const BalanceChange &change,
                             messages::BlockHeight height);

 public:
  LedgerMongodb(const std::string &url, const std::string &db_name);
  LedgerMongodb(const std::string &url, const std::string &db_name,
                const messages::Block &block0);
  explicit LedgerMongodb(const messages::config::Database &config);

  ~LedgerMongodb();

  bool is_ancestor(const messages::BranchPath &ancestor_path,
                   const messages::BranchPath &block_path) const;

  bool is_ancestor(const messages::TaggedBlock &ancestor,
                   const messages::TaggedBlock &block) const;

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

  bool get_tagged_block_balances(const messages::BlockID &id,
                                 messages::TaggedBlock *tagged_block) const;

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

  bool insert_block(const messages::TaggedBlock &tagged_block);

  bool insert_block(const messages::Block &block);

  bool delete_block(const messages::BlockID &id);

  bool delete_block_and_children(const messages::BlockID &id);

  bool get_transaction(const messages::TransactionID &id,
                       messages::Transaction *transaction) const;

  bool get_transaction(const messages::TransactionID &id,
                       messages::Transaction *transaction,
                       messages::BlockHeight *blockheight) const;

  bool get_transaction(const messages::TransactionID &id,
                       messages::TaggedTransaction *tagged_transaction,
                       const messages::TaggedBlock &tip,
                       bool include_transaction_pool) const;

  std::vector<messages::TaggedTransaction> get_transactions(
      const messages::TransactionID &id, const messages::TaggedBlock &tip,
      bool include_transaction_pool) const;

  std::size_t total_nb_transactions() const;

  std::size_t total_nb_blocks() const;

  bool for_each(const Filter &filter, const messages::TaggedBlock &tip,
                bool include_transaction_pool, Functor functor) const;

  bool for_each(const Filter &filter, Functor functor) const;

  int new_branch_id();

  bool add_transaction(const messages::TaggedTransaction &tagged_transaction);

  bool add_to_transaction_pool(const messages::Transaction &transaction);

  bool delete_transaction(const messages::TransactionID &id);

  std::vector<messages::TaggedTransaction> get_transaction_pool() const;

  std::size_t get_transaction_pool(messages::Block *block) const;

  bool get_unverified_blocks(
      std::vector<messages::TaggedBlock> *tagged_blocks) const;

  bool set_block_verified(const messages::BlockID &id,
                          const messages::BlockScore &score,
                          const messages::AssemblyID previous_assembly_id);

  bool update_main_branch();

  messages::BranchPath fork_from(const messages::BranchPath &branch_path) const;

  messages::BranchPath first_child(
      const messages::BranchPath &branch_path) const;

  bool get_pii(const messages::_KeyPub &key_pub,
               const messages::AssemblyID &assembly_id, Double *pii) const;

  bool get_assembly_piis(const messages::AssemblyID &assembly_id,
                         std::vector<messages::Pii> *piis);

  void empty_database();

  void init_database(const messages::Block &block0);

  bool get_assembly(const messages::AssemblyID &assembly_id,
                    messages::Assembly *assembly) const;

  bool get_assembly(const messages::AssemblyHeight &height,
                    messages::Assembly *assembly) const;

  bool add_assembly(const messages::TaggedBlock &tagged_block,
                    const messages::AssemblyHeight height);

  bool set_pii(const messages::Pii &pii);

  bool set_integrity(const messages::Integrity &integrity);

  messages::IntegrityScore get_integrity(
      const messages::_KeyPub &key_pub,
      const messages::AssemblyHeight &assembly_height,
      const messages::BranchPath &branch_path) const;

  bool add_integrity(const messages::_KeyPub &key_pub,
                     const messages::AssemblyID &assembly_id,
                     const messages::AssemblyHeight &assembly_height,
                     const messages::BranchPath &branch_path,
                     const messages::IntegrityScore &added_score);

  bool set_previous_assembly_id(
      const messages::BlockID &block_id,
      const messages::AssemblyID &previous_assembly_id);

  bool get_next_assembly(const messages::AssemblyID &assembly_id,
                         messages::Assembly *assembly) const;

  bool get_assemblies_to_compute(
      std::vector<messages::Assembly> *assemblies) const;

  bool get_block_writer(const messages::AssemblyID &assembly_id,
                        int32_t key_pub_rank, messages::_KeyPub *key_pub) const;

  bool set_nb_key_pubs(const messages::AssemblyID &assembly_id,
                       int32_t nb_key_pubs);

  bool set_seed(const messages::AssemblyID &assembly_id, int32_t seed);

  bool set_finished_computation(const messages::AssemblyID &assembly_id);

  bool denunciation_exists(const messages::Denunciation &denunciation,
                           const messages::BlockHeight &max_block_height,
                           const messages::BranchPath &branch_path) const;

  messages::TaggedBlocks get_blocks(const messages::BlockHeight height,
                                    const messages::_KeyPub &author,
                                    bool include_transactions = true) const;
  messages::TaggedBlocks get_blocks(const messages::Branch name) const;

  void add_double_mining(
      const std::vector<messages::TaggedBlock> &tagged_blocks);

  std::vector<messages::Denunciation> get_double_minings() const;

  void add_denunciations(messages::Block *block,
                         const messages::BranchPath &branch_path) const;

  void add_denunciations(
      messages::Block *block, const messages::BranchPath &branch_path,
      const std::vector<messages::Denunciation> &denunciations) const;

  messages::Balance get_balance(
      const messages::_KeyPub &key_pub,
      const messages::TaggedBlock &tagged_block) const;

  bool add_balances(messages::TaggedBlock *tagged_block);

  friend class neuro::ledger::tests::LedgerMongodb;
};

}  // namespace ledger
}  // namespace neuro

#endif /* NEURO_SRC_LEDGERMONGODB_HPP */
