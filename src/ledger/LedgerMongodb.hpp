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

class LedgerMongodb : public Ledger {
 private:
  static mongocxx::instance _instance;
  mutable mongocxx::uri _uri;
  mutable mongocxx::client _client;
  mutable mongocxx::database _db;
  mutable mongocxx::collection _blocks;
  mutable mongocxx::collection _transactions;
  mutable mongocxx::collection _blocks_forks;

  std::mutex _ledger_mutex;

  mongocxx::options::find remove_OID();

  bool get_block_header(const bsoncxx::document::view &block,
                        messages::BlockHeader *header);

  bool get_transactions_from_block(const bsoncxx::document::view &id,
                                   messages::Block *block);

  bool get_transactions_from_block(const messages::BlockID &id,
                                   messages::Block *block);

  bool init_block0(const messages::config::Database &config);

  bool get_block_unsafe(const messages::BlockID &id, messages::Block *block);

  bool get_block_unsafe(const messages::BlockHeight height,
                        messages::Block *block);

  bool get_block_header_unsafe(const messages::BlockID &id,
                               messages::BlockHeader *header);

 public:
  LedgerMongodb(const std::string &url, const std::string &db_name);
  LedgerMongodb(const messages::config::Database &config);

  ~LedgerMongodb() {}

  void remove_all();

  messages::BlockHeight height();

  bool get_block_header(const messages::BlockID &id,
                        messages::BlockHeader *header);

  bool get_last_block_header(messages::BlockHeader *block_header);

  bool get_block(const messages::BlockID &id, messages::Block *block);

  bool get_block_by_previd(const messages::BlockID &previd,
                           messages::Block *block);

  bool get_block(const messages::BlockHeight height, messages::Block *block);

  bool push_block(const messages::Block &block);

  bool delete_block(const messages::Hash &id);

  bool get_transaction(const messages::Hash &id,
                       messages::Transaction *transaction);

  bool get_transaction(const messages::Hash &id,
                       messages::Transaction *transaction,
                       messages::BlockHeight *blockheight);
  bool add_transaction(const messages::Transaction &transaction);

  bool delete_transaction(const messages::Hash &id);

  int get_transaction_pool(messages::Block *block);

  int total_nb_transactions();

  int total_nb_blocks();

  bool get_blocks(int start, int size, std::vector<messages::Block> &blocks);

  bool for_each(const Filter &filter, Functor functor);

  /**
   * Functions for forks
   */

  bool fork_add_block(const messages::Block &block);

  bool fork_delete_block(const messages::Hash &id);

  bool fork_get_block(const messages::BlockID &id, messages::Block *block);

  bool fork_get_block(const messages::BlockHeight height,
                      messages::Block *block);

  bool fork_get_block_by_previd(const messages::BlockID &previd,
                                messages::Block *block);

  void fork_for_each(Functor_block functor);
  void fork_test();
};

}  // namespace ledger
}  // namespace neuro

#endif /* NEURO_SRC_LEDGERMONGODB_HPP */
