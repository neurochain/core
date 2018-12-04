#include "ledger/LedgerMongodb.hpp"
#include <gtest/gtest.h>
#include <chrono>
#include <sstream>
#include <thread>
#include "Bot.hpp"
#include "common/logger.hpp"
#include "common/types.hpp"
#include "messages/config/Config.hpp"
#include "messages/config/Database.hpp"
#include "tooling/genblock.hpp"

namespace neuro {
namespace ledger {
namespace tests {

class LedgerMongodb : public ::testing::Test {
 public:
  const std::string config_str =
      "{"
      "  \"url\": \"mongodb://mongo:27017\","
      "  \"dbName\": \"faucet\","
      "  \"block0Format\": \"PROTO\","
      "  \"block0Path\": \"./data.0.testnet\""
      "}";

 protected:
  messages::config::Database _config;
  std::shared_ptr<::neuro::ledger::LedgerMongodb> ledger;

  void create_empty_db(const messages::config::Database &config) {
    // instance started in static in LedgerMongodb
    mongocxx::uri uri(config.url());
    mongocxx::client client(uri);
    mongocxx::database db(client[config.db_name()]);
    db.drop();
  }

  void create_first_blocks(const int nb) {
    for (int i = 1; i < nb; ++i) {
      messages::Block block;
      tooling::genblock::genblock_from_last_db_block(block, ledger, 1, i);

      messages::TaggedBlock tagged_block;
      tagged_block.add_branch_path(0);
      tagged_block.set_branch(messages::Branch::MAIN);
      ledger->insert_block(&tagged_block);
    }
  }

  LedgerMongodb() : _config(config_str) {
    create_empty_db(_config);
    ledger = std::make_shared<::neuro::ledger::LedgerMongodb>(_config);
  }

  std::string db_name() const { return _config.db_name(); }

  std::string db_url() const { return _config.url(); }

  messages::config::Database config() const { return _config; }
};

TEST_F(LedgerMongodb, load_block_0) {
  // messages::Block block;
  // ASSERT_TRUE(ledger->get_block(0, &block));
  // ASSERT_FALSE(ledger->get_block_by_previd(block.header().id(), &block));
}

/*TEST_F(LedgerMongodb, load_block_1_to_9) {*/
// create_first_blocks(10);
// ASSERT_EQ(9, ledger->height());
//}

// TEST_F(LedgerMongodb, header) {
// create_first_blocks(2);
// messages::Block block;
// messages::BlockHeader header;
// messages::BlockHeader last_header;
// ASSERT_EQ(1, ledger->height());
// ledger->get_block(1, &block);
// ASSERT_TRUE(ledger->get_block_header(block.header().id(), &header));
// ASSERT_TRUE(ledger->get_last_block_header(&last_header));
// ASSERT_EQ(block.header(), header);
// ASSERT_EQ(last_header, header);
//}

// TEST_F(LedgerMongodb, remove_all) {
// create_first_blocks(10);
// ledger->remove_all();
// ASSERT_EQ(0, ledger->height());
//}

// TEST_F(LedgerMongodb, get_block) {
// messages::Block block0, block0bis, block1, block7;
// create_first_blocks(10);
// ASSERT_EQ(10, ledger->total_nb_blocks());

// ledger->get_block(0, &block0);
// const auto id0 = block0.header().id();
// ASSERT_TRUE(ledger->get_block_by_previd(id0, &block1));
// ASSERT_TRUE(ledger->get_block(id0, &block0bis));
// std::vector<messages::Block> blocks;
//// ledger->get_blocks(2, 8, blocks);
// ledger->get_block(7, &block7);
// ASSERT_EQ(block7, blocks[5]);
// ASSERT_EQ(block0, block0bis);
//}

// TEST_F(LedgerMongodb, delete_block) {
// messages::Block block0;
// ledger->get_block(0, &block0);
// ASSERT_EQ(1, ledger->total_nb_blocks());
// ASSERT_EQ(0, ledger->height());
// ASSERT_TRUE(ledger->delete_block(block0.header().id()));
// ASSERT_EQ(0, ledger->total_nb_blocks());
// ASSERT_FALSE(ledger->delete_block(block0.header().id()));
// ASSERT_FALSE(ledger->get_block(0, &block0));
// ASSERT_EQ(0, ledger->height());
//}

// TEST_F(LedgerMongodb, transactions) {
// messages::Block block0;
// ledger->get_block(0, &block0);

// ASSERT_EQ(4, ledger->total_nb_transactions());
// ASSERT_EQ(4, block0.transactions().size());
// const auto transaction0 = block0.transactions(0);

// messages::Transaction transaction0bis;
// ASSERT_TRUE(ledger->get_transaction(transaction0.id(), &transaction0bis));
// ASSERT_EQ(transaction0, transaction0bis);

// int height = 42;
// ASSERT_TRUE(
// ledger->get_transaction(transaction0.id(), &transaction0bis, &height));
// ASSERT_EQ(height, 0);
// ledger->delete_transaction(transaction0bis.id());
// ASSERT_FALSE(ledger->get_transaction(transaction0.id(), &transaction0bis,
// 0));

// messages::TaggedTransaction tagged_transaction;
//*tagged_transaction.mutable_transaction() = transaction0;
// ledger->add_transaction(tagged_transaction);
// ASSERT_TRUE(ledger->get_transaction(transaction0.id(), &transaction0bis));
// ASSERT_EQ(transaction0, transaction0bis);
//}

// TEST_F(LedgerMongodb, push) {
// messages::Block block, block_fake;
// create_first_blocks(2);
// ASSERT_TRUE(ledger->get_block(1, &block));
// ASSERT_TRUE(ledger->delete_block(block.header().id()));
// ASSERT_FALSE(ledger->delete_block(block.header().id()));
// ASSERT_FALSE(ledger->get_block(block.header().id(), &block_fake));
//// ASSERT_TRUE(ledger->push_block(block));
//// TODO should not allow to insert twice the same block
//// ASSERT_FALSE(ledger->push_block(block));
//}

// TEST_F(LedgerMongodb, forks) {
// create_first_blocks(10);
// messages::Block block0;
// messages::Block block0fork;
// ledger->get_block(0, &block0);
//// ASSERT_FALSE(ledger->fork_get_block(block0.header().id(), &block0fork));
//// ASSERT_TRUE(ledger->fork_add_block(block0));
//// ASSERT_TRUE(ledger->fork_get_block(block0.header().id(), &block0fork));
//// TODO Should not be accepted since block exist in non fork collection

//// TODO should not allow to insert twice the same block
//// ASSERT_FALSE(ledger->fork_add_block(block5));

//// ASSERT_TRUE(ledger->fork_delete_block(block0.header().id()));
//// ASSERT_FALSE(ledger->fork_delete_block(block0.header().id()));
/*}*/

}  // namespace tests
}  // namespace ledger
}  // namespace neuro
