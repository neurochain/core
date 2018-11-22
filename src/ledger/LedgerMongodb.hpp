#ifndef NEURO_SRC_LEDGERMONGODB_HPP
#define NEURO_SRC_LEDGERMONGODB_HPP

#include <mutex>

#include "config.pb.h"
#include "ledger/Ledger.hpp"
#include "ledger/mongo.hpp"
#include "messages.pb.h"

namespace neuro {
namespace ledger {

class LedgerMongodb : public Ledger {
 private:
  static mongocxx::instance _instance;
  mutable mongocxx::uri _uri;
  mutable mongocxx::client _client;
  mutable mongocxx::database _db;
  mutable mongocxx::collection _blocks;

  std::mutex _ledger_mutex;

  mongocxx::options::find remove_OID();

  mongocxx::options::find projection(std::string field);

  mongocxx::options::find projection(std::string field0, std::string field1);

  void init_block0(const messages::config::Database &db);

  MongoQuery query_branch(const messages::Branch &branch);

  MongoQuery query_main_branch();

 public:
  LedgerMongodb(const std::string &url, const std::string &db_name);
  LedgerMongodb(const messages::config::Database &db);

  ~LedgerMongodb() {}

  void remove_all();

  messages::BlockHeight height();

  bool get_block_header(const messages::BlockID &id,
                        messages::BlockHeader *header);

  bool get_last_block_header(messages::BlockHeader *block_header);

  bool get_block(const messages::BlockID &id,
                 messages::TaggedBlock *tagged_block);

  bool get_block(const messages::BlockID &id, messages::Block *block);

  bool get_block_by_previd(const messages::BlockID &previd,
                           messages::Block *block);

  bool get_blocks_by_previd(const messages::BlockID &previd,
                            std::vector<messages::TaggedBlock> *tagged_blocks);

  bool get_block(const messages::BlockHeight height, messages::Block *block);

  bool insert_block(const messages::Block &block,
                    const messages::Branch &branch);

  bool push_block(const messages::Block &block);

  bool delete_block(const messages::Hash &id);

  bool get_transaction(const messages::Hash &id,
                       messages::Transaction *transaction);

  bool get_transaction(const messages::Hash &id,
                       messages::Transaction *transaction,
                       messages::BlockHeight *blockheight);

  int total_nb_transactions();

  int total_nb_blocks();

  bool get_blocks(int start, int size, std::vector<messages::Block> &blocks);

  bool for_each(const Filter &filter, Functor functor);
};

}  // namespace ledger
}  // namespace neuro

#endif /* NEURO_SRC_LEDGERMONGODB_HPP */
