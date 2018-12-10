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

std::vector<messages::BranchID> to_vector(
    google::protobuf::RepeatedField<messages::BranchID> branch_path) {
  std::vector<messages::BranchID> result;
  for (const auto &branch_id : branch_path) {
    result.push_back(branch_id);
  }
  return result;
}

class LedgerMongodb : public ::testing::Test {
 public:
  const std::string config_str =
      "{"
      "  \"url\": \"mongodb://mongo:27017\","
      "  \"dbName\": \"test_ledger\","
      "  \"block0Format\": \"PROTO\","
      "  \"block0Path\": \"./data.0.testnet\""
      "}";

 protected:
  messages::config::Database _config;
  std::shared_ptr<::neuro::ledger::LedgerMongodb> ledger;

  LedgerMongodb() : _config(config_str) {
    tooling::genblock::create_empty_db(_config);
    ledger = std::make_shared<::neuro::ledger::LedgerMongodb>(_config);
  }

  std::string db_name() const { return _config.db_name(); }

  std::string db_url() const { return _config.url(); }

  messages::config::Database config() const { return _config; }
};

TEST_F(LedgerMongodb, load_block_0) {
  messages::Block block;
  ASSERT_TRUE(ledger->get_block(0, &block));
  ASSERT_FALSE(ledger->get_block_by_previd(block.header().id(), &block));
}

TEST_F(LedgerMongodb, load_block_1_to_9) {
  tooling::genblock::create_first_blocks(10, ledger);
  ASSERT_EQ(9, ledger->height());
}

TEST_F(LedgerMongodb, header) {
  tooling::genblock::create_first_blocks(2, ledger);
  messages::Block block;
  messages::BlockHeader header;
  messages::BlockHeader last_header;
  ASSERT_EQ(1, ledger->height());
  ledger->get_block(1, &block);
  ASSERT_TRUE(ledger->get_block_header(block.header().id(), &header));
  ASSERT_TRUE(ledger->get_last_block_header(&last_header));
  ASSERT_EQ(block.header(), header);
  ASSERT_EQ(last_header, header);
}

TEST_F(LedgerMongodb, remove_all) {
  tooling::genblock::create_first_blocks(10, ledger);
  ledger->remove_all();
  ASSERT_EQ(0, ledger->height());
}

TEST_F(LedgerMongodb, get_block) {
  messages::Block block0, block0bis, block1, block7;
  tooling::genblock::create_first_blocks(10, ledger);
  ASSERT_EQ(10, ledger->total_nb_blocks());

  ledger->get_block(0, &block0);
  const auto id0 = block0.header().id();
  ASSERT_TRUE(ledger->get_block_by_previd(id0, &block1));
  ASSERT_TRUE(ledger->get_block(id0, &block0bis));
}

TEST_F(LedgerMongodb, delete_block) {
  messages::Block block0;
  ledger->get_block(0, &block0);
  ASSERT_EQ(1, ledger->total_nb_blocks());
  ASSERT_EQ(0, ledger->height());
  ASSERT_TRUE(ledger->delete_block(block0.header().id()));
  ASSERT_EQ(0, ledger->total_nb_blocks());
  ASSERT_FALSE(ledger->delete_block(block0.header().id()));
  ASSERT_FALSE(ledger->get_block(0, &block0));
  ASSERT_EQ(0, ledger->height());
}

TEST_F(LedgerMongodb, transactions) {
  messages::Block block0;
  ledger->get_block(0, &block0);

  ASSERT_EQ(2, ledger->total_nb_transactions());
  ASSERT_EQ(2, block0.transactions().size());
  const auto transaction0 = block0.transactions(0);

  messages::Transaction transaction0bis;
  ASSERT_TRUE(ledger->get_transaction(transaction0.id(), &transaction0bis));
  ASSERT_EQ(transaction0, transaction0bis);

  int height = 42;
  ASSERT_TRUE(
      ledger->get_transaction(transaction0.id(), &transaction0bis, &height));
  ASSERT_EQ(height, 0);
  ASSERT_FALSE(ledger->delete_transaction(transaction0bis.id()));

  // Add a transaction to the transaction pool
  messages::TaggedTransaction tagged_transaction;
  *tagged_transaction.mutable_transaction() = transaction0;
  ledger->add_transaction(tagged_transaction);
  messages::Block block;
  ledger->get_transaction_pool(&block);
  ASSERT_EQ(block.transactions_size(), 1);
  ASSERT_TRUE(ledger->delete_transaction(transaction0.id()));
  block.Clear();
  ledger->get_transaction_pool(&block);
  ASSERT_EQ(block.transactions_size(), 0);
}

TEST_F(LedgerMongodb, insert_tagged_block) {
  messages::Block block, fake_block;
  tooling::genblock::create_first_blocks(2, ledger);
  ASSERT_TRUE(ledger->get_block(1, &block));
  ASSERT_TRUE(ledger->delete_block(block.header().id()));
  ASSERT_FALSE(ledger->delete_block(block.header().id()));
  ASSERT_FALSE(ledger->get_block(block.header().id(), &fake_block));
  messages::TaggedBlock tagged_block;
  tagged_block.set_branch(messages::Branch::MAIN);
  tagged_block.add_branch_path(0);
  *tagged_block.mutable_block() = block;
  ASSERT_TRUE(ledger->insert_block(&tagged_block));
  ASSERT_FALSE(ledger->insert_block(&tagged_block));
  fake_block.Clear();
  ASSERT_TRUE(ledger->get_block(block.header().id(), &fake_block));
}

TEST_F(LedgerMongodb, insert_block) {
  messages::Block block0, block1, block2;
  messages::TaggedBlock tagged_block;
  ASSERT_TRUE(ledger->get_block(ledger->height(), &block0));
  tooling::genblock::genblock_from_block(&block1, block0, 1);
  tooling::genblock::genblock_from_block(&block2, block1, 2);
  ASSERT_TRUE(ledger->insert_block(&block1));
  ASSERT_TRUE(ledger->insert_block(&block2));
  ASSERT_TRUE(ledger->get_block(block1.header().id(), &tagged_block));
  ASSERT_EQ(tagged_block.branch(), messages::Branch::FORK);
  ASSERT_EQ(to_vector(tagged_block.branch_path()),
            std::vector<messages::BranchID>{0});
  tagged_block.Clear();
  ASSERT_TRUE(ledger->get_block(block2.header().id(), &tagged_block));
  ASSERT_EQ(tagged_block.branch(), messages::Branch::FORK);
  ASSERT_EQ(to_vector(tagged_block.branch_path()),
            std::vector<messages::BranchID>{0});
}

TEST_F(LedgerMongodb, insert_block_attach) {
  messages::Block block0, block1, block2;
  messages::TaggedBlock tagged_block;
  ASSERT_TRUE(ledger->get_block(ledger->height(), &block0));
  tooling::genblock::genblock_from_block(&block1, block0, 1);
  tooling::genblock::genblock_from_block(&block2, block1, 2);
  ASSERT_TRUE(ledger->insert_block(&block2));
  ASSERT_TRUE(ledger->get_block(block2.header().id(), &tagged_block));
  ASSERT_EQ(tagged_block.branch(), messages::Branch::DETACHED);
  ASSERT_EQ(to_vector(tagged_block.branch_path()),
            std::vector<messages::BranchID>{});
  ASSERT_TRUE(ledger->insert_block(&block1));
  tagged_block.Clear();
  ASSERT_TRUE(ledger->get_block(block1.header().id(), &tagged_block));
  ASSERT_EQ(tagged_block.branch(), messages::Branch::FORK);
  ASSERT_EQ(to_vector(tagged_block.branch_path()),
            std::vector<messages::BranchID>{0});
  tagged_block.Clear();
  ASSERT_TRUE(ledger->get_block(block2.header().id(), &tagged_block));
  ASSERT_EQ(tagged_block.branch(), messages::Branch::FORK);
  ASSERT_EQ(to_vector(tagged_block.branch_path()),
            std::vector<messages::BranchID>{0});
}

TEST_F(LedgerMongodb, branch_path) {
  messages::Block block0, block1, block2, fork1, fork2;
  messages::TaggedBlock tagged_block;
  ASSERT_TRUE(ledger->get_block(ledger->height(), &block0));
  tooling::genblock::genblock_from_block(&block1, block0, 1);
  tooling::genblock::genblock_from_block(&block2, block1, 2);

  // Use a different height so that the fork has a different id
  tooling::genblock::genblock_from_block(&fork1, block0, 2);
  tooling::genblock::genblock_from_block(&fork2, fork1, 3);

  // Insert the main branch
  ASSERT_TRUE(ledger->insert_block(&block1));
  ASSERT_TRUE(ledger->insert_block(&block2));

  // Insert the fork branch
  ASSERT_TRUE(ledger->insert_block(&fork1));
  ASSERT_TRUE(ledger->insert_block(&fork2));

  // Check the branch path
  ASSERT_TRUE(ledger->get_block(block2.header().id(), &tagged_block));
  ASSERT_EQ(tagged_block.branch(), messages::Branch::FORK);
  ASSERT_EQ(to_vector(tagged_block.branch_path()),
            std::vector<messages::BranchID>{0});
  tagged_block.Clear();
  ASSERT_TRUE(ledger->get_block(fork2.header().id(), &tagged_block));
  ASSERT_EQ(tagged_block.branch(), messages::Branch::FORK);
  auto branch_path = std::vector<messages::BranchID>{1};
  branch_path.push_back(0);
  ASSERT_EQ(to_vector(tagged_block.branch_path()), branch_path);
}

TEST_F(LedgerMongodb, set_block_score) {
  messages::Block block0;
  messages::TaggedBlock tagged_block;
  ASSERT_TRUE(ledger->get_block(ledger->height(), &block0));
  ASSERT_TRUE(ledger->set_block_score(block0.header().id(), 17));
  ASSERT_TRUE(ledger->get_block(block0.header().id(), &tagged_block));
  ASSERT_EQ(tagged_block.score(), 17);
}

}  // namespace tests
}  // namespace ledger
}  // namespace neuro
