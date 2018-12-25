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
#include "tooling/blockgen.hpp"

namespace neuro {
namespace ledger {
namespace tests {

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
    tooling::blockgen::create_empty_db(_config);
    ledger = std::make_shared<::neuro::ledger::LedgerMongodb>(_config);
  }

  std::string db_name() const { return _config.db_name(); }

  std::string db_url() const { return _config.url(); }

  messages::config::Database config() const { return _config; }

 public:
  void test_is_ancestor() {
    messages::TaggedBlock block0, block1, block2, fork0, fork1;
    auto branch_path = block0.mutable_branch_path();
    branch_path->add_branch_ids(0);
    branch_path->add_block_numbers(0);
    branch_path = block1.mutable_branch_path();
    branch_path->add_branch_ids(0);
    branch_path->add_block_numbers(1);
    branch_path = block2.mutable_branch_path();
    branch_path->add_branch_ids(0);
    branch_path->add_block_numbers(2);
    branch_path = fork0.mutable_branch_path();
    branch_path->add_branch_ids(1);
    branch_path->add_branch_ids(0);
    branch_path->add_block_numbers(0);
    branch_path->add_block_numbers(1);
    branch_path = fork1.mutable_branch_path();
    branch_path->add_branch_ids(1);
    branch_path->add_branch_ids(0);
    branch_path->add_block_numbers(1);
    branch_path->add_block_numbers(1);

    // Block0
    ASSERT_TRUE(ledger->is_ancestor(block0, block0));
    ASSERT_TRUE(ledger->is_ancestor(block0, block1));
    ASSERT_TRUE(ledger->is_ancestor(block0, block2));
    ASSERT_TRUE(ledger->is_ancestor(block0, fork0));
    ASSERT_TRUE(ledger->is_ancestor(block0, fork1));

    // Block1
    ASSERT_FALSE(ledger->is_ancestor(block1, block0));
    ASSERT_TRUE(ledger->is_ancestor(block1, block1));
    ASSERT_TRUE(ledger->is_ancestor(block1, block2));
    ASSERT_TRUE(ledger->is_ancestor(block1, fork0));
    ASSERT_TRUE(ledger->is_ancestor(block1, fork1));

    // Block2
    ASSERT_FALSE(ledger->is_ancestor(block2, block0));
    ASSERT_FALSE(ledger->is_ancestor(block2, block1));
    ASSERT_TRUE(ledger->is_ancestor(block2, block2));
    ASSERT_FALSE(ledger->is_ancestor(block2, fork0));
    ASSERT_FALSE(ledger->is_ancestor(block2, fork1));

    // Fork0
    ASSERT_FALSE(ledger->is_ancestor(fork0, block0));
    ASSERT_FALSE(ledger->is_ancestor(fork0, block1));
    ASSERT_FALSE(ledger->is_ancestor(fork0, block2));
    ASSERT_TRUE(ledger->is_ancestor(fork0, fork0));
    ASSERT_TRUE(ledger->is_ancestor(fork0, fork1));

    // Fork1
    ASSERT_FALSE(ledger->is_ancestor(fork1, block0));
    ASSERT_FALSE(ledger->is_ancestor(fork1, block1));
    ASSERT_FALSE(ledger->is_ancestor(fork1, block2));
    ASSERT_FALSE(ledger->is_ancestor(fork1, fork0));
    ASSERT_TRUE(ledger->is_ancestor(fork1, fork1));
  }
};

TEST_F(LedgerMongodb, load_block_0) {
  messages::Block block;
  ASSERT_TRUE(ledger->get_block(0, &block));
  ASSERT_FALSE(ledger->get_block_by_previd(block.header().id(), &block));
}

TEST_F(LedgerMongodb, load_block_1_to_9) {
  tooling::blockgen::append_blocks(9, ledger);
  ASSERT_EQ(9, ledger->height());
}

TEST_F(LedgerMongodb, header) {
  tooling::blockgen::append_blocks(1, ledger);
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
  tooling::blockgen::append_blocks(9, ledger);
  ledger->remove_all();
  ASSERT_EQ(0, ledger->height());
}

TEST_F(LedgerMongodb, get_block) {
  messages::Block block0, block0bis, block1, block7;
  tooling::blockgen::append_blocks(9, ledger);
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
  ASSERT_EQ(0, block0.transactions().size());
  ASSERT_EQ(2, block0.coinbases().size());
  const auto transaction0 = block0.coinbases(0);

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
  tagged_transaction.mutable_transaction()->CopyFrom(transaction0);
  tagged_transaction.set_is_coinbase(true);
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
  tooling::blockgen::append_blocks(2, ledger);
  ASSERT_TRUE(ledger->get_block(1, &block));
  ASSERT_TRUE(ledger->delete_block(block.header().id()));
  ASSERT_FALSE(ledger->delete_block(block.header().id()));
  ASSERT_FALSE(ledger->get_block(block.header().id(), &fake_block));
  messages::TaggedBlock tagged_block;
  tagged_block.set_branch(messages::Branch::MAIN);
  tagged_block.mutable_branch_path()->add_branch_ids(0);
  tagged_block.mutable_branch_path()->add_block_numbers(1);
  tagged_block.mutable_block()->CopyFrom(block);
  ASSERT_TRUE(ledger->insert_block(&tagged_block));
  ASSERT_FALSE(ledger->insert_block(&tagged_block));
  fake_block.Clear();
  ASSERT_TRUE(ledger->get_block(block.header().id(), &fake_block));
}

TEST_F(LedgerMongodb, insert_block) {
  messages::Block block0, block1, block2;
  messages::TaggedBlock tagged_block;
  ASSERT_TRUE(ledger->get_block(ledger->height(), &block0));
  tooling::blockgen::blockgen_from_block(&block1, block0, 1);
  tooling::blockgen::blockgen_from_block(&block2, block1, 2);
  ASSERT_TRUE(ledger->insert_block(&block1));
  ASSERT_TRUE(ledger->insert_block(&block2));
  ASSERT_TRUE(ledger->get_block(block1.header().id(), &tagged_block));
  ASSERT_EQ(tagged_block.branch(), messages::Branch::FORK);
  messages::BranchPath branch_path;
  branch_path.add_branch_ids(0);
  branch_path.add_block_numbers(1);
  ASSERT_EQ(tagged_block.branch_path(), branch_path);
  tagged_block.Clear();
  ASSERT_TRUE(ledger->get_block(block2.header().id(), &tagged_block));
  ASSERT_EQ(tagged_block.branch(), messages::Branch::FORK);
  branch_path.Clear();
  branch_path.add_branch_ids(0);
  branch_path.add_block_numbers(2);
  ASSERT_EQ(tagged_block.branch_path(), branch_path);
}

TEST_F(LedgerMongodb, insert_block_attach) {
  messages::Block block0, block1, block2;
  messages::TaggedBlock tagged_block;
  ASSERT_TRUE(ledger->get_block(ledger->height(), &block0));
  tooling::blockgen::blockgen_from_block(&block1, block0, 1);
  tooling::blockgen::blockgen_from_block(&block2, block1, 2);
  ASSERT_TRUE(ledger->insert_block(&block2));
  ASSERT_TRUE(ledger->get_block(block2.header().id(), &tagged_block));
  ASSERT_EQ(tagged_block.branch(), messages::Branch::DETACHED);
  ASSERT_FALSE(tagged_block.has_branch_path());
  ASSERT_TRUE(ledger->insert_block(&block1));
  tagged_block.Clear();
  ASSERT_TRUE(ledger->get_block(block1.header().id(), &tagged_block));
  ASSERT_EQ(tagged_block.branch(), messages::Branch::FORK);
  messages::BranchPath branch_path;
  branch_path.add_branch_ids(0);
  branch_path.add_block_numbers(1);
  ASSERT_EQ(tagged_block.branch_path(), branch_path);
  tagged_block.Clear();
  ASSERT_TRUE(ledger->get_block(block2.header().id(), &tagged_block));
  ASSERT_EQ(tagged_block.branch(), messages::Branch::FORK);
  branch_path.Clear();
  branch_path.add_branch_ids(0);
  branch_path.add_block_numbers(2);
  ASSERT_EQ(tagged_block.branch_path(), branch_path);
}

TEST_F(LedgerMongodb, branch_path) {
  messages::Block block0, block1, block2, fork1, fork2;
  ASSERT_TRUE(ledger->get_block(0, &block0));
  tooling::blockgen::blockgen_from_block(&block1, block0, 1);
  tooling::blockgen::blockgen_from_block(&block2, block1, 2);

  // Use a different height so that the fork has a different id
  tooling::blockgen::blockgen_from_block(&fork1, block0, 2);
  tooling::blockgen::blockgen_from_block(&fork2, fork1, 3);

  // Insert the main branch
  ASSERT_TRUE(ledger->insert_block(&block1));
  ASSERT_TRUE(ledger->insert_block(&block2));

  // Insert the fork branch
  ASSERT_TRUE(ledger->insert_block(&fork1));
  ASSERT_TRUE(ledger->insert_block(&fork2));

  // Check the branch path
  messages::TaggedBlock tagged_block;
  ASSERT_TRUE(ledger->get_block(block2.header().id(), &tagged_block));
  ASSERT_EQ(tagged_block.branch(), messages::Branch::FORK);
  messages::BranchPath branch_path;
  branch_path.add_branch_ids(0);
  branch_path.add_block_numbers(2);
  ASSERT_EQ(tagged_block.branch_path(), branch_path);
  tagged_block.Clear();
  ASSERT_TRUE(ledger->get_block(fork2.header().id(), &tagged_block));
  ASSERT_EQ(tagged_block.branch(), messages::Branch::FORK);
  branch_path.Clear();
  branch_path.add_branch_ids(1);
  branch_path.add_branch_ids(0);
  branch_path.add_block_numbers(1);
  branch_path.add_block_numbers(0);
  ASSERT_EQ(tagged_block.branch_path(), branch_path);
}

TEST_F(LedgerMongodb, set_block_score) {
  messages::Block block0;
  messages::TaggedBlock tagged_block;
  ASSERT_TRUE(ledger->get_block(ledger->height(), &block0));
  ASSERT_TRUE(ledger->set_block_score(block0.header().id(), 17));
  ASSERT_TRUE(ledger->get_block(block0.header().id(), &tagged_block));
  ASSERT_EQ(tagged_block.score(), 17);
}

TEST_F(LedgerMongodb, get_unscored_forks) {
  messages::Block block0, block1, block2, fork1, fork2;
  ASSERT_TRUE(ledger->get_block(0, &block0));
  tooling::blockgen::blockgen_from_block(&block1, block0, 1);
  tooling::blockgen::blockgen_from_block(&block2, block1, 2);

  // Use a different height so that the fork has a different id
  tooling::blockgen::blockgen_from_block(&fork1, block0, 2);
  tooling::blockgen::blockgen_from_block(&fork2, fork1, 3);

  // Insert the block2 which should be detached
  ASSERT_TRUE(ledger->insert_block(&block2));
  messages::TaggedBlock tagged_block;
  ASSERT_TRUE(ledger->get_block(block2.header().id(), &tagged_block));
  ASSERT_EQ(tagged_block.branch(), messages::Branch::DETACHED);
  std::vector<messages::TaggedBlock> unscored_forks;
  ASSERT_TRUE(ledger->get_unscored_forks(&unscored_forks));
  ASSERT_EQ(unscored_forks.size(), 0);

  // Insert the block1 which should attach the block2
  ASSERT_TRUE(ledger->insert_block(&block1));
  tagged_block.Clear();
  ASSERT_TRUE(ledger->get_block(block1.header().id(), &tagged_block));
  ASSERT_EQ(tagged_block.branch(), messages::Branch::FORK);
  ASSERT_TRUE(ledger->get_block(block2.header().id(), &tagged_block));
  ASSERT_EQ(tagged_block.branch(), messages::Branch::FORK);
  unscored_forks.clear();
  ASSERT_TRUE(ledger->get_unscored_forks(&unscored_forks));
  ASSERT_EQ(unscored_forks.size(), 2);

  ASSERT_TRUE(ledger->set_block_score(block1.header().id(), 1.17));
  unscored_forks.clear();
  ASSERT_TRUE(ledger->get_unscored_forks(&unscored_forks));
  ASSERT_EQ(unscored_forks.size(), 1);

  ASSERT_TRUE(ledger->set_block_score(block2.header().id(), 2.17));
  unscored_forks.clear();
  ASSERT_TRUE(ledger->get_unscored_forks(&unscored_forks));
  ASSERT_EQ(unscored_forks.size(), 0);
}

TEST_F(LedgerMongodb, update_main_branch) {
  messages::Block block0, block1, block2, fork1, fork2, fork3;
  ASSERT_TRUE(ledger->get_block(0, &block0));
  tooling::blockgen::blockgen_from_block(&block1, block0, 1);
  tooling::blockgen::blockgen_from_block(&block2, block1, 2);

  // Use a different height so that the fork has a different id
  tooling::blockgen::blockgen_from_block(&fork1, block0, 2);
  tooling::blockgen::blockgen_from_block(&fork2, fork1, 3);
  tooling::blockgen::blockgen_from_block(&fork3, fork2, 4);

  // Insert the main branch
  ASSERT_TRUE(ledger->insert_block(&block1));
  ASSERT_TRUE(ledger->insert_block(&block2));

  // Insert the fork branch
  ASSERT_TRUE(ledger->insert_block(&fork1));
  ASSERT_TRUE(ledger->insert_block(&fork2));

  ASSERT_TRUE(ledger->set_block_score(block0.header().id(), 1));
  ASSERT_TRUE(ledger->set_block_score(block1.header().id(), 2));
  ASSERT_TRUE(ledger->set_block_score(block2.header().id(), 3));
  ASSERT_TRUE(ledger->set_block_score(fork1.header().id(), 2));
  ASSERT_TRUE(ledger->set_block_score(fork2.header().id(), 2.5));

  messages::TaggedBlock tip, tagged_block;
  ASSERT_TRUE(ledger->update_main_branch(&tip));
  ASSERT_EQ(tip.block().header().id(), block2.header().id());
  ASSERT_EQ(tip.branch(), messages::Branch::MAIN);
  ASSERT_TRUE(ledger->get_block(block2.header().id(), &tagged_block));
  ASSERT_EQ(tagged_block.branch(), messages::Branch::MAIN);
  ASSERT_TRUE(ledger->get_block(block1.header().id(), &tagged_block));
  ASSERT_EQ(tagged_block.branch(), messages::Branch::MAIN);
  ASSERT_TRUE(ledger->get_block(fork1.header().id(), &tagged_block));
  ASSERT_EQ(tagged_block.branch(), messages::Branch::FORK);
  ASSERT_TRUE(ledger->get_block(fork2.header().id(), &tagged_block));
  ASSERT_EQ(tagged_block.branch(), messages::Branch::FORK);

  // Switch MAIN branch
  ASSERT_TRUE(ledger->insert_block(&fork3));
  ASSERT_TRUE(ledger->set_block_score(fork3.header().id(), 3.5));
  tip.Clear();
  ASSERT_TRUE(ledger->update_main_branch(&tip));
  ASSERT_EQ(tip.block().header().id(), fork3.header().id());
  ASSERT_EQ(tip.branch(), messages::Branch::MAIN);
  ASSERT_TRUE(ledger->get_block(fork3.header().id(), &tagged_block));
  ASSERT_EQ(tagged_block.branch(), messages::Branch::MAIN);
  ASSERT_TRUE(ledger->get_block(fork2.header().id(), &tagged_block));
  ASSERT_EQ(tagged_block.branch(), messages::Branch::MAIN);
  ASSERT_TRUE(ledger->get_block(fork1.header().id(), &tagged_block));
  ASSERT_EQ(tagged_block.branch(), messages::Branch::MAIN);
  ASSERT_TRUE(ledger->get_block(block1.header().id(), &tagged_block));
  ASSERT_EQ(tagged_block.branch(), messages::Branch::FORK);
  ASSERT_TRUE(ledger->get_block(block2.header().id(), &tagged_block));
  ASSERT_EQ(tagged_block.branch(), messages::Branch::FORK);
}

TEST_F(LedgerMongodb, is_ancestor) { test_is_ancestor(); }

}  // namespace tests
}  // namespace ledger
}  // namespace neuro
