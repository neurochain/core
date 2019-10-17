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
#include "tooling/Simulator.hpp"
#include "tooling/blockgen.hpp"

namespace neuro {
namespace ledger {
namespace tests {

template <typename M>
int cursor_size(Cursor<M> cursor) {
  int size = 0;
  auto it = cursor.begin();
  while (it != cursor.end()) {
    ++it;
    size++;
  }
  return size;
}

class LedgerMongodb : public ::testing::Test {
 public:
  const std::string db_url = "mongodb://mongo:27017";
  const std::string db_name = "test_ledger";
  const messages::NCCAmount ncc_block0 = messages::NCCAmount(1000000);
  const int nb_keys = 2;
  tooling::Simulator simulator;
  std::shared_ptr<::neuro::ledger::LedgerMongodb> ledger;

  LedgerMongodb()
      : simulator(tooling::Simulator::Simulator::StaticSimulator(
            db_url, db_name, nb_keys, ncc_block0)),
        ledger(simulator.ledger) {}

 public:
  void test_is_ancestor() {
    messages::TaggedBlock block0, block1, block2, fork0, fork1;
    auto branch_path = block0.mutable_branch_path();
    branch_path->add_branch_ids(0);
    branch_path->add_block_numbers(0);
    branch_path->set_branch_id(0);
    branch_path->set_block_number(0);
    branch_path = block1.mutable_branch_path();
    branch_path->add_branch_ids(0);
    branch_path->add_block_numbers(1);
    branch_path->set_branch_id(0);
    branch_path->set_block_number(1);
    branch_path = block2.mutable_branch_path();
    branch_path->add_branch_ids(0);
    branch_path->add_block_numbers(2);
    branch_path->set_branch_id(0);
    branch_path->set_block_number(2);
    branch_path = fork0.mutable_branch_path();
    branch_path->add_branch_ids(1);
    branch_path->add_branch_ids(0);
    branch_path->add_block_numbers(0);
    branch_path->add_block_numbers(1);
    branch_path->set_branch_id(1);
    branch_path->set_block_number(0);
    branch_path = fork1.mutable_branch_path();
    branch_path->add_branch_ids(1);
    branch_path->add_branch_ids(0);
    branch_path->add_block_numbers(1);
    branch_path->add_block_numbers(1);
    branch_path->set_branch_id(1);
    branch_path->set_block_number(1);

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

  void test_update_main_branch() {
    messages::Block block0, block1, block2, fork1, fork2, fork3;
    ASSERT_TRUE(ledger->get_block(0, &block0));
    tooling::blockgen::blockgen_from_block(&block1, block0, 1);
    tooling::blockgen::blockgen_from_block(&block2, block1, 2);

    // Use a different height so that the fork has a different id
    tooling::blockgen::blockgen_from_block(&fork1, block0, 2);
    tooling::blockgen::blockgen_from_block(&fork2, fork1, 3);
    tooling::blockgen::blockgen_from_block(&fork3, fork2, 4);

    // Insert the main branch
    ASSERT_TRUE(ledger->insert_block(block1));
    ASSERT_TRUE(ledger->insert_block(block2));

    // Insert the fork branch
    ASSERT_TRUE(ledger->insert_block(fork1));
    ASSERT_TRUE(ledger->insert_block(fork2));

    ledger->update_branch_tag(block0.header().id(), messages::Branch::MAIN);
    ASSERT_TRUE(ledger->set_block_verified(block1.header().id(), 20,
                                           block0.header().id()));
    ASSERT_TRUE(ledger->set_block_verified(block2.header().id(), 30,
                                           block0.header().id()));
    ASSERT_TRUE(ledger->set_block_verified(fork1.header().id(), 20,
                                           block0.header().id()));
    ASSERT_TRUE(ledger->set_block_verified(fork2.header().id(), 25,
                                           block0.header().id()));

    ASSERT_TRUE(ledger->update_main_branch());
    auto tip = ledger->get_main_branch_tip();
    ASSERT_EQ(tip.block().header().id(), block2.header().id());
    ASSERT_EQ(tip.branch(), messages::Branch::MAIN);
    messages::TaggedBlock tagged_block;
    ASSERT_TRUE(ledger->get_block(block2.header().id(), &tagged_block));
    ASSERT_EQ(tagged_block.branch(), messages::Branch::MAIN);
    ASSERT_TRUE(ledger->get_block(block1.header().id(), &tagged_block));
    ASSERT_EQ(tagged_block.branch(), messages::Branch::MAIN);
    ASSERT_TRUE(ledger->get_block(fork1.header().id(), &tagged_block));
    ASSERT_EQ(tagged_block.branch(), messages::Branch::FORK);
    ASSERT_TRUE(ledger->get_block(fork2.header().id(), &tagged_block));
    ASSERT_EQ(tagged_block.branch(), messages::Branch::FORK);

    // Switch MAIN branch
    ASSERT_TRUE(ledger->insert_block(fork3));
    ASSERT_TRUE(ledger->set_block_verified(fork3.header().id(), 35,
                                           block0.header().id()));
    tip.Clear();
    ASSERT_TRUE(ledger->update_main_branch());
    tip = ledger->get_main_branch_tip();
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

  void test_clean_transaction_pool() {
    auto &key_pub1 = simulator.key_pubs[1];

    // Check that a block does not empty the transaction pool
    {
      auto cursor = ledger->get_transaction_pool();
      ASSERT_TRUE(cursor.begin() == cursor.end());
    }
    auto block1 = simulator.new_block();
    ASSERT_TRUE(simulator.consensus->add_transaction(
        ledger->send_ncc(simulator.keys[0].key_priv(), key_pub1, 0.5)));
    {
      auto transactions = ledger->get_transaction_pool();
      auto it = transactions.begin();

      ASSERT_FALSE(it == transactions.end());
      ++it;
      ASSERT_TRUE(it == transactions.end());
    }
    ASSERT_TRUE(simulator.consensus->add_block(block1));

    {
      auto transactions = ledger->get_transaction_pool();
      auto it = transactions.begin();
      ASSERT_FALSE(it == transactions.end());
      ++it;
      ASSERT_TRUE(it == transactions.end());
    }
    ASSERT_EQ(ledger->cleanup_transaction_pool(), 1);
    {
      auto transactions = ledger->get_transaction_pool();
      ASSERT_TRUE(transactions.begin() == transactions.end());
    }
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

  ASSERT_EQ(1, ledger->total_nb_transactions());
  ASSERT_EQ(0, block0.transactions().size());
  ASSERT_EQ(nb_keys, block0.coinbase().outputs_size());
  const auto transaction0 = block0.coinbase();

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
  ledger->get_transaction_pool(&block, 256000, 300);
  ASSERT_EQ(block.transactions_size(), 1);
  ASSERT_TRUE(ledger->delete_transaction(transaction0.id()));
  block.Clear();
  ledger->get_transaction_pool(&block, 256000, 300);
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
  tagged_block.mutable_branch_path()->set_branch_id(0);
  tagged_block.mutable_branch_path()->set_block_number(1);
  tagged_block.mutable_block()->CopyFrom(block);
  ASSERT_TRUE(ledger->insert_block(tagged_block));
  ASSERT_FALSE(ledger->insert_block(tagged_block));
  fake_block.Clear();
  ASSERT_TRUE(ledger->get_block(block.header().id(), &fake_block));
}

TEST_F(LedgerMongodb, insert_block) {
  messages::Block block0, block1, block2;
  messages::TaggedBlock tagged_block;
  ASSERT_TRUE(ledger->get_block(ledger->height(), &block0));
  tooling::blockgen::blockgen_from_block(&block1, block0, 1);
  tooling::blockgen::blockgen_from_block(&block2, block1, 2);
  ASSERT_TRUE(ledger->insert_block(block1));
  ASSERT_TRUE(ledger->insert_block(block2));
  ASSERT_TRUE(ledger->get_block(block1.header().id(), &tagged_block));
  ASSERT_EQ(tagged_block.branch(), messages::Branch::UNVERIFIED);
  messages::BranchPath branch_path;
  branch_path.add_branch_ids(0);
  branch_path.add_block_numbers(1);
  branch_path.set_branch_id(0);
  branch_path.set_block_number(1);
  ASSERT_EQ(tagged_block.branch_path(), branch_path);
  tagged_block.Clear();
  ASSERT_TRUE(ledger->get_block(block2.header().id(), &tagged_block));
  ASSERT_EQ(tagged_block.branch(), messages::Branch::UNVERIFIED);
  branch_path.Clear();
  branch_path.add_branch_ids(0);
  branch_path.add_block_numbers(2);
  branch_path.set_branch_id(0);
  branch_path.set_block_number(2);
  ASSERT_EQ(tagged_block.branch_path(), branch_path);
}

TEST_F(LedgerMongodb, insert_block_attach) {
  messages::Block block0, block1, block2;
  messages::TaggedBlock tagged_block;
  ASSERT_TRUE(ledger->get_block(ledger->height(), &block0));
  tooling::blockgen::blockgen_from_block(&block1, block0, 1);
  tooling::blockgen::blockgen_from_block(&block2, block1, 2);
  ASSERT_TRUE(ledger->insert_block(block2));
  ASSERT_TRUE(ledger->get_block(block2.header().id(), &tagged_block));
  ASSERT_EQ(tagged_block.branch(), messages::Branch::DETACHED);
  ASSERT_FALSE(tagged_block.has_branch_path());
  ASSERT_TRUE(ledger->insert_block(block1));
  tagged_block.Clear();
  ASSERT_TRUE(ledger->get_block(block1.header().id(), &tagged_block));
  ASSERT_EQ(tagged_block.branch(), messages::Branch::UNVERIFIED);
  messages::BranchPath branch_path;
  branch_path.add_branch_ids(0);
  branch_path.add_block_numbers(1);
  branch_path.set_branch_id(0);
  branch_path.set_block_number(1);
  ASSERT_EQ(tagged_block.branch_path(), branch_path);
  tagged_block.Clear();
  ASSERT_TRUE(ledger->get_block(block2.header().id(), &tagged_block));
  ASSERT_EQ(tagged_block.branch(), messages::Branch::UNVERIFIED);
  branch_path.Clear();
  branch_path.add_branch_ids(0);
  branch_path.add_block_numbers(2);
  branch_path.set_branch_id(0);
  branch_path.set_block_number(2);
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
  ASSERT_TRUE(ledger->insert_block(block1));
  ASSERT_TRUE(ledger->insert_block(block2));

  // Insert the fork branch
  ASSERT_TRUE(ledger->insert_block(fork1));
  ASSERT_TRUE(ledger->insert_block(fork2));

  // Check the branch path
  messages::TaggedBlock tagged_block;
  ASSERT_TRUE(ledger->get_block(block2.header().id(), &tagged_block));
  ASSERT_EQ(tagged_block.branch(), messages::Branch::UNVERIFIED);
  messages::BranchPath branch_path;
  branch_path.add_branch_ids(0);
  branch_path.add_block_numbers(2);
  branch_path.set_branch_id(0);
  branch_path.set_block_number(2);
  ASSERT_EQ(tagged_block.branch_path(), branch_path);
  tagged_block.Clear();
  ASSERT_TRUE(ledger->get_block(fork2.header().id(), &tagged_block));
  ASSERT_EQ(tagged_block.branch(), messages::Branch::UNVERIFIED);
  branch_path.Clear();
  branch_path.add_branch_ids(1);
  branch_path.add_branch_ids(0);
  branch_path.add_block_numbers(1);
  branch_path.add_block_numbers(0);
  branch_path.set_branch_id(1);
  branch_path.set_block_number(1);
  ASSERT_EQ(tagged_block.branch_path(), branch_path);
}

TEST_F(LedgerMongodb, set_block_verified) {
  messages::TaggedBlock tagged_block;
  messages::Block block0;
  ASSERT_TRUE(ledger->get_block(0, &block0));
  ASSERT_TRUE(ledger->get_block(ledger->height(), &block0));
  ASSERT_TRUE(ledger->set_block_verified(block0.header().id(), 17,
                                         block0.header().id()));
  ASSERT_TRUE(ledger->get_block(block0.header().id(), &tagged_block));
  ASSERT_EQ(tagged_block.score(), 17);
}

TEST_F(LedgerMongodb, get_unverified_blocks) {
  messages::Block block0, block1, block2, fork1, fork2;
  ASSERT_TRUE(ledger->get_block(0, &block0));
  tooling::blockgen::blockgen_from_block(&block1, block0, 1);
  tooling::blockgen::blockgen_from_block(&block2, block1, 2);

  // Use a different height so that the fork has a different id
  tooling::blockgen::blockgen_from_block(&fork1, block0, 2);
  tooling::blockgen::blockgen_from_block(&fork2, fork1, 3);

  // Insert the block2 which should be detached
  ASSERT_TRUE(ledger->insert_block(block2));
  messages::TaggedBlock tagged_block;
  ASSERT_TRUE(ledger->get_block(block2.header().id(), &tagged_block));
  ASSERT_EQ(tagged_block.branch(), messages::Branch::DETACHED);
  std::vector<messages::TaggedBlock> unscored_forks;
  ASSERT_EQ(cursor_size(ledger->get_unverified_blocks()), 0);

  // Insert the block1 which should attach the block2
  ASSERT_TRUE(ledger->insert_block(block1));
  tagged_block.Clear();
  ASSERT_TRUE(ledger->get_block(block1.header().id(), &tagged_block));
  ASSERT_EQ(tagged_block.branch(), messages::Branch::UNVERIFIED);
  ASSERT_TRUE(ledger->get_block(block2.header().id(), &tagged_block));
  ASSERT_EQ(tagged_block.branch(), messages::Branch::UNVERIFIED);
  unscored_forks.clear();
  ASSERT_EQ(cursor_size(ledger->get_unverified_blocks()), 2);

  ASSERT_TRUE(ledger->set_block_verified(block1.header().id(), 1,
                                         block0.header().id()));
  unscored_forks.clear();
  ASSERT_EQ(cursor_size(ledger->get_unverified_blocks()), 1);

  ASSERT_TRUE(ledger->set_block_verified(block2.header().id(), 2,
                                         block0.header().id()));
  unscored_forks.clear();
  ASSERT_EQ(cursor_size(ledger->get_unverified_blocks()), 0);
}

TEST_F(LedgerMongodb, update_main_branch) { test_update_main_branch(); }

TEST_F(LedgerMongodb, is_ancestor) { test_is_ancestor(); }

TEST_F(LedgerMongodb, empty_database) {
  ASSERT_EQ(ledger->total_nb_blocks(), 1);
  ledger->empty_database();
  ASSERT_EQ(ledger->total_nb_blocks(), 0);
}

TEST_F(LedgerMongodb, assembly) {
  messages::TaggedBlock tagged_block;
  ASSERT_TRUE(ledger->get_block(0, &tagged_block));
  messages::Assembly assembly;
  ASSERT_FALSE(
      ledger->get_assembly(tagged_block.block().header().id(), &assembly));
  ASSERT_TRUE(ledger->add_assembly(tagged_block, 0));
  ASSERT_TRUE(
      ledger->get_assembly(tagged_block.block().header().id(), &assembly));
  ASSERT_EQ(assembly.id(), tagged_block.block().header().id());
  ASSERT_TRUE(assembly.has_previous_assembly_id());
  ASSERT_FALSE(assembly.finished_computation());

  // set_nb_key_pubs
  ASSERT_FALSE(assembly.has_nb_key_pubs());
  ASSERT_TRUE(ledger->set_nb_key_pubs(assembly.id(), 17));
  ASSERT_TRUE(
      ledger->get_assembly(tagged_block.block().header().id(), &assembly));
  ASSERT_EQ(assembly.nb_key_pubs(), 17);

  // set_seed
  ASSERT_FALSE(assembly.has_seed());
  ASSERT_TRUE(ledger->set_seed(assembly.id(), 19));
  ASSERT_TRUE(
      ledger->get_assembly(tagged_block.block().header().id(), &assembly));
  ASSERT_EQ(assembly.seed(), 19);

  // get_assemblies_to_compute
  std::vector<messages::Assembly> assemblies;
  ASSERT_TRUE(ledger->get_assemblies_to_compute(&assemblies));
  ASSERT_EQ(assemblies.size(), 1);
}

TEST_F(LedgerMongodb, pii) {
  messages::TaggedBlock tagged_block;
  ASSERT_TRUE(ledger->get_block(0, &tagged_block));
  ASSERT_TRUE(ledger->add_assembly(tagged_block, 0));
  auto assembly_id = tagged_block.block().header().id();
  std::vector<crypto::Ecc> keys{5};
  for (int i = 0; i < 5; i++) {
    messages::Pii pii;
    pii.mutable_key_pub()->CopyFrom(messages::_KeyPub(keys[i].key_pub()));
    pii.mutable_assembly_id()->CopyFrom(assembly_id);
    pii.set_score(std::to_string(10 - i));
    pii.set_rank(i);
    ASSERT_TRUE(ledger->set_pii(pii));
  }
  for (int i = 0; i < 5; i++) {
    messages::_KeyPub key_pub;
    ASSERT_TRUE(ledger->get_block_writer(assembly_id, i, &key_pub));
    ASSERT_EQ(key_pub, messages::_KeyPub(keys[i].key_pub()));
  }
}

TEST_F(LedgerMongodb, integrity) {
  messages::Integrity integrity;
  crypto::Ecc ecc;
  integrity.mutable_key_pub()->CopyFrom(ecc.key_pub());

  // It does not really matter yet I need a hash for the assembly id
  messages::TaggedBlock block0;
  ASSERT_TRUE(ledger->get_last_block(&block0));
  ASSERT_EQ(block0.block().header().height(), 0);
  integrity.mutable_assembly_id()->CopyFrom(block0.block().header().id());
  integrity.set_assembly_height(0);
  integrity.set_block_height(0);
  integrity.set_score("17");
  integrity.mutable_branch_path()->CopyFrom(block0.branch_path());
  ASSERT_TRUE(ledger->set_integrity(integrity));

  auto assembly_height = 0;
  auto integrity_score = ledger->get_integrity(
      integrity.key_pub(), assembly_height, block0.branch_path());
  ASSERT_EQ(integrity_score, 17);

  auto block = simulator.new_block();
  simulator.consensus->add_block(block);
  messages::TaggedBlock block1;
  ASSERT_TRUE(ledger->get_last_block(&block1));
  ASSERT_EQ(block1.block().header().height(), 1);

  assembly_height = 1;
  integrity_score = ledger->get_integrity(integrity.key_pub(), assembly_height,
                                          block1.branch_path());
  ASSERT_EQ(integrity_score, 17);

  integrity.set_assembly_height(1);
  integrity.set_block_height(block1.block().header().height());
  integrity.set_score("18");
  integrity.mutable_branch_path()->CopyFrom(block1.branch_path());
  ASSERT_TRUE(ledger->set_integrity(integrity));

  // We should get the new integrity when looking at this assembly height
  assembly_height = 1;
  integrity_score = ledger->get_integrity(integrity.key_pub(), assembly_height,
                                          block1.branch_path());
  ASSERT_EQ(integrity_score, 18);

  // We should get the old integrity when looking at this assembly height
  assembly_height = 0;
  integrity_score = ledger->get_integrity(integrity.key_pub(), assembly_height,
                                          block0.branch_path());
  ASSERT_EQ(integrity_score, 17);

  // And if we look in a different branch path that forked at block0 we should
  // get the old integrity
  messages::BranchPath branch_path;
  branch_path.add_branch_ids(1);
  branch_path.add_block_numbers(0);
  branch_path.add_branch_ids(0);
  branch_path.add_block_numbers(0);
  branch_path.set_branch_id(1);
  branch_path.set_block_number(0);
  assembly_height = 1;
  integrity_score =
      ledger->get_integrity(integrity.key_pub(), assembly_height, branch_path);
  ASSERT_EQ(integrity_score, 17);
}

TEST_F(LedgerMongodb, set_previous_assembly_id) {
  messages::TaggedBlock tagged_block;
  ASSERT_TRUE(ledger->get_block(0, &tagged_block));
  auto block_id = tagged_block.block().header().id();
  auto assembly_id = block_id;
  messages::Assembly assembly;
  ASSERT_TRUE(
      ledger->get_assembly(tagged_block.previous_assembly_id(), &assembly));
  ASSERT_TRUE(ledger->get_assembly(assembly.previous_assembly_id(), &assembly));
  ASSERT_TRUE(tagged_block.has_previous_assembly_id());
  ASSERT_NE(tagged_block.previous_assembly_id(), assembly_id);
  ASSERT_TRUE(ledger->set_previous_assembly_id(block_id, assembly_id));
  ASSERT_TRUE(ledger->get_block(0, &tagged_block));
  ASSERT_TRUE(tagged_block.has_previous_assembly_id());
  ASSERT_EQ(tagged_block.previous_assembly_id(), assembly_id);
}

TEST_F(LedgerMongodb, list_transactions) {
  auto &key_pub = simulator.key_pubs[0];
  auto transactions = ledger->list_transactions(key_pub).transactions();
  ASSERT_EQ(transactions.size(), 1);
  ASSERT_EQ(transactions[0].outputs(0).key_pub(), key_pub);

  // Check that we only get incoming transactions
  // Send everything so that there is no change output going back to the key_pub
  auto transaction =
      ledger->send_ncc(simulator.keys[0].key_priv(), simulator.key_pubs[1], 1);
  simulator.consensus->add_transaction(transaction);
  transactions = ledger->list_transactions(key_pub).transactions();
  ASSERT_EQ(transactions.size(), 1);

  // Check that we do get transactions from the transaction pool
  transaction = ledger->send_ncc(simulator.keys[1].key_priv(),
                                 simulator.key_pubs[0], 0.5);
  simulator.consensus->add_transaction(transaction);
  transactions = ledger->list_transactions(key_pub).transactions();
  ASSERT_EQ(transactions.size(), 2);

  // Putting the transactions in a block should not change anything
  auto block = simulator.new_block();
  bool has_coinbase = block.coinbase().outputs(0).key_pub() == key_pub;
  simulator.consensus->add_block(block);
  transactions = ledger->list_transactions(key_pub).transactions();
  ASSERT_EQ(transactions.size(), has_coinbase ? 3 : 2);
}

TEST_F(LedgerMongodb, get_outputs_for_key_pub) {
  auto transaction = ledger->send_ncc(simulator.keys[0].key_priv(),
                                      simulator.key_pubs[1], 0.5);
  simulator.consensus->add_transaction(transaction);
  auto outputs =
      ledger->get_outputs_for_key_pub(transaction.id(), simulator.key_pubs[1]);
  ASSERT_EQ(outputs.size(), 1);
  ASSERT_EQ(outputs.at(0).output_id(), 0);
  ASSERT_EQ(outputs.at(0).key_pub(), simulator.key_pubs[1]);
  outputs =
      ledger->get_outputs_for_key_pub(transaction.id(), simulator.key_pubs[0]);

  // There is no change output
  ASSERT_EQ(outputs.size(), 0);
}

TEST_F(LedgerMongodb, has_received_transaction) {
  ASSERT_TRUE(ledger->has_received_transaction(simulator.key_pubs[0]));
  crypto::Ecc ecc;
  auto key_pub = ecc.key_pub();
  ASSERT_FALSE(ledger->has_received_transaction(key_pub));
  auto transaction =
      ledger->send_ncc(simulator.keys[0].key_priv(), key_pub, 0.5);
  simulator.consensus->add_transaction(transaction);
  ASSERT_TRUE(ledger->has_received_transaction(key_pub));
}

TEST_F(LedgerMongodb, get_last_blocks) {
  for (int i = 0; i < 2; i++) {
    auto new_block = simulator.new_block();
    simulator.consensus->add_block(new_block);
  }
  auto blocks = ledger->get_last_blocks(2);
  ASSERT_EQ(blocks.size(), 2);
  ASSERT_EQ(blocks.at(0).header().height(), 2);
  ASSERT_EQ(blocks.at(1).header().height(), 1);
}

TEST_F(LedgerMongodb, balance) {
  auto &key_pub0 = simulator.key_pubs[0];
  auto &key_pub1 = simulator.key_pubs[1];
  ASSERT_EQ(ledger->balance(key_pub0).value(), ncc_block0.value());
  ASSERT_EQ(ledger->balance(key_pub1).value(), ncc_block0.value());
  ASSERT_TRUE(simulator.consensus->add_transaction(
      ledger->send_ncc(simulator.keys[0].key_priv(), key_pub1, 1)));
  auto block = simulator.new_block();
  ASSERT_EQ(block.transactions_size(), 1);
  bool author_index = block.coinbase().outputs(0).key_pub() != key_pub0;
  ASSERT_TRUE(simulator.consensus->add_block(block));
  {
    auto transactions = ledger->get_transaction_pool();
    ASSERT_TRUE(transactions.begin() == transactions.end());
  }
  int balance0 = 100 * (!author_index);
  int balance1 = 2 * ncc_block0.value() + 100 * author_index;
  ASSERT_EQ(ledger->balance(key_pub0).value(), balance0);
  ASSERT_EQ(ledger->balance(key_pub1).value(), balance1);
  ASSERT_TRUE(simulator.consensus->add_transaction(
      ledger->send_ncc(simulator.keys[1].key_priv(), key_pub0, 0.5)));
  block = simulator.new_block();
  ASSERT_EQ(block.transactions_size(), 1);
  author_index = block.coinbase().outputs(0).key_pub() != key_pub0;
  balance0 += 0.5 * balance1 + 100 * (!author_index);
  balance1 = 0.5 * balance1 + 100 * author_index;
  ASSERT_TRUE(simulator.consensus->add_block(simulator.new_block()));
  ASSERT_EQ(ledger->balance(key_pub0).value(), balance0);
  ASSERT_EQ(ledger->balance(key_pub1).value(), balance1);
}

TEST_F(LedgerMongodb, denunciation_exists) {
  // Let's make the first miner double mine
  auto block1 = simulator.new_block();
  auto block1_bis = simulator.new_block(1);
  ASSERT_TRUE(simulator.consensus->add_block(block1));
  ASSERT_TRUE(simulator.consensus->add_block(block1_bis));

  // Let's denounce the vile double miner
  auto block2 = simulator.new_block();
  ASSERT_EQ(block2.header().previous_block_hash(), block1.header().id());
  block2.clear_denunciations();
  auto denunciation = block2.add_denunciations();
  denunciation->mutable_block_id()->CopyFrom(block1_bis.header().id());
  denunciation->set_block_height(1);
  denunciation->mutable_block_author()->CopyFrom(block1_bis.header().author());

  // The hash and the signatures need to be fixed now that we changed the block
  uint32_t miner_index = simulator.key_pubs_indexes[messages::_KeyPub(
      block2.header().author().key_pub())];
  messages::set_block_hash(&block2);
  crypto::sign(simulator.keys[miner_index], &block2);

  auto block2_bis = simulator.new_block();
  block2_bis.clear_denunciations();
  messages::set_block_hash(&block2_bis);
  crypto::sign(simulator.keys[miner_index], &block2_bis);

  ASSERT_TRUE(simulator.consensus->add_block(block2));
  ASSERT_TRUE(simulator.consensus->add_block(block2_bis));

  messages::TaggedBlock tagged_block2, tagged_block2_bis;
  ASSERT_TRUE(ledger->get_block(block2.header().id(), &tagged_block2));
  ASSERT_TRUE(ledger->get_block(block2_bis.header().id(), &tagged_block2_bis));
  ASSERT_FALSE(ledger->denunciation_exists(*denunciation,
                                           block2.header().height() - 1,
                                           tagged_block2.branch_path()));
  ASSERT_FALSE(ledger->denunciation_exists(*denunciation,
                                           block2_bis.header().height() - 1,
                                           tagged_block2_bis.branch_path()));

  auto block3 = simulator.new_block();
  ASSERT_TRUE(simulator.consensus->add_block(block3));
  messages::TaggedBlock tagged_block3;
  ASSERT_TRUE(ledger->get_block(3, &tagged_block3));
  ASSERT_TRUE(ledger->denunciation_exists(*denunciation,
                                          block3.header().height() - 1,
                                          tagged_block3.branch_path()));

  auto block3_bis = simulator.new_block(tagged_block2_bis);
  block3_bis.clear_denunciations();

  // The hash and the signatures need to be fixed now that we changed the block
  miner_index = simulator.key_pubs_indexes[messages::_KeyPub(
      block3_bis.header().author().key_pub())];
  messages::set_block_hash(&block3_bis);
  crypto::sign(simulator.keys[miner_index], &block3_bis);
  messages::set_block_hash(&block3_bis);
  crypto::sign(simulator.keys[miner_index], &block3_bis);

  ASSERT_EQ(block3_bis.header().height(), 3);
  ASSERT_EQ(block3_bis.header().previous_block_hash(),
            block2_bis.header().id());
  ASSERT_TRUE(simulator.consensus->add_block(block3_bis));
  messages::TaggedBlock tagged_block3_bis;
  ASSERT_TRUE(ledger->get_block(block3_bis.header().id(), &tagged_block3_bis));
  ASSERT_FALSE(ledger->denunciation_exists(*denunciation,
                                           block3_bis.header().height() - 1,
                                           tagged_block3_bis.branch_path()));
}

TEST_F(LedgerMongodb, get_blocks) {
  // Let's make the first miner double mine
  auto block1 = simulator.new_block();
  auto block1_bis = simulator.new_block(1);
  ASSERT_TRUE(simulator.consensus->add_block(block1));

  auto include_transactions = true;
  auto blocks = ledger->get_blocks(block1.header().height(),
                                   block1.header().author().key_pub(),
                                   include_transactions);
  ASSERT_EQ(blocks.size(), 1);
  ASSERT_EQ(blocks.at(0).block(), block1);

  ASSERT_TRUE(simulator.consensus->add_block(block1_bis));
  blocks = ledger->get_blocks(block1.header().height(),
                              block1.header().author().key_pub(),
                              include_transactions);
  ASSERT_EQ(blocks.size(), 2);
  ASSERT_TRUE(blocks.at(0).block() == block1 ||
              blocks.at(0).block() == block1_bis);
  ASSERT_TRUE(blocks.at(1).block() == block1 ||
              blocks.at(1).block() == block1_bis);
}

TEST_F(LedgerMongodb, double_minings) {
  // Let's make the first miner double mine
  auto block1 = simulator.new_block();
  auto block1_bis = simulator.new_block(1);
  ASSERT_TRUE(simulator.consensus->add_block(block1));

  ASSERT_TRUE(simulator.consensus->add_block(block1_bis));
  bool include_transactions = false;
  auto blocks = ledger->get_blocks(block1.header().height(),
                                   block1.header().author().key_pub(),
                                   include_transactions);
  ASSERT_EQ(blocks.size(), 2);
  ledger->add_double_mining(blocks);
  auto denunciations = ledger->get_double_minings();
  ASSERT_EQ(denunciations.size(), 2);

  for (const auto &denunciation : denunciations) {
    const auto &block_id = denunciation.block_id();
    ASSERT_TRUE(block_id == block1.header().id() ||
                block_id == block1_bis.header().id());
    ASSERT_EQ(denunciation.block_author().key_pub(),
              block1.header().author().key_pub());
    ASSERT_EQ(denunciation.block_height(), 1);
    const auto &branch_path = denunciation.branch_path();
    ASSERT_TRUE(branch_path == blocks[0].branch_path() ||
                branch_path == blocks[1].branch_path());
  }

  // Check that adding them again does not create duplicates
  ledger->add_double_mining(blocks);
  denunciations = ledger->get_double_minings();
  ASSERT_EQ(denunciations.size(), 2);
}

TEST_F(LedgerMongodb, add_denunciations) {
  // Let's make the first miner double mine
  auto block1 = simulator.new_block();
  auto block1_bis = simulator.new_block(1);
  ASSERT_TRUE(simulator.consensus->add_block(block1));

  ASSERT_TRUE(simulator.consensus->add_block(block1_bis));
  auto block = simulator.new_block();
  block.clear_denunciations();
  messages::TaggedBlock previous;
  bool include_transactions = false;
  ASSERT_TRUE(ledger->get_block(block.header().previous_block_hash(), &previous,
                                include_transactions));
  ledger->add_denunciations(&block, previous.branch_path());
  ASSERT_EQ(block.denunciations_size(), 1);
  auto &denunciation = block.denunciations(0);
  ASSERT_EQ(denunciation.block_id(), block1_bis.header().id());
  ASSERT_EQ(denunciation.block_height(), block1_bis.header().height());
  ASSERT_EQ(denunciation.block_author(), block1_bis.header().author());
  ASSERT_FALSE(denunciation.has_branch_path());
}

TEST_F(LedgerMongodb, send_ncc) {
  auto &key_pub0 = simulator.key_pubs[0];
  auto &key_pub1 = simulator.key_pubs[1];
  ASSERT_EQ(ledger->balance(key_pub0), ncc_block0);
  ASSERT_EQ(ledger->balance(key_pub1), ncc_block0);
  ASSERT_TRUE(simulator.consensus->add_transaction(
      ledger->send_ncc(simulator.keys[0].key_priv(), key_pub1, 0.5)));
  auto block = simulator.new_block();
  ASSERT_EQ(block.transactions_size(), 1);
  bool author_index = block.coinbase().outputs(0).key_pub() != key_pub0;
  ASSERT_TRUE(simulator.consensus->add_block(block));
  messages::NCCValue balance0 =
      100 * (!author_index) + 0.5 * ncc_block0.value();
  ASSERT_EQ(ledger->balance(key_pub0).value(), balance0);
  ASSERT_TRUE(simulator.consensus->add_transaction(
      ledger->send_ncc(simulator.keys[0].key_priv(), key_pub1, 1)));
  block = simulator.new_block();
  ASSERT_EQ(block.transactions_size(), 1);
  author_index = block.coinbase().outputs(0).key_pub() != key_pub0;
  ASSERT_TRUE(simulator.consensus->add_block(block));
  balance0 = 100 * (!author_index);
  ASSERT_EQ(ledger->balance(key_pub0).value(), balance0);
}

TEST_F(LedgerMongodb, clean_transaction_pool) { test_clean_transaction_pool(); }

TEST_F(LedgerMongodb, get_balance) {
  // Write 5 blocks in the main branch
  // with 1 address on block 1, 2 on block 2, 3 on block 3 ...
  // the balance is equal to the height of the block
  std::vector<crypto::Ecc> eccs{5};
  std::vector<messages::TaggedBlock> tagged_blocks;
  ASSERT_TRUE(ledger->get_last_block(&tagged_blocks.emplace_back()));
  for (int i = 1; i < 6; i++) {
    auto &previous = tagged_blocks[i - 1];
    messages::TaggedBlock tagged_block;
    tooling::blockgen::blockgen_from_block(tagged_block.mutable_block(),
                                           previous.block(), i);
    auto branch_path = tagged_block.mutable_branch_path();
    branch_path->add_branch_ids(0);
    branch_path->add_block_numbers(i);
    branch_path->set_branch_id(0);
    branch_path->set_block_number(i);
    tagged_block.set_branch(messages::Branch::MAIN);
    for (int j = 0; j < i; j++) {
      auto balance = tagged_block.add_balances();
      balance->mutable_key_pub()->CopyFrom(eccs[j].key_pub());
      balance->mutable_value()->set_value(i);
      balance->set_enthalpy_begin("0");
      balance->set_enthalpy_end("0");
    }
    ASSERT_TRUE(ledger->insert_block(tagged_block));
    tagged_blocks.push_back(tagged_block);
  }
  for (int i = 0; i < 6; i++) {
    for (int j = 0; j < 5; j++) {
      auto balance = ledger->get_balance(eccs[j].key_pub(), tagged_blocks[i]);
      if (i > 0 && j < i) {
        ASSERT_EQ(balance.value().value(), i);
      } else {
        ASSERT_EQ(balance.value().value(), 0);
      }
    }
  }

  // Write 5 blocks in a new branch starting at block 3
  std::vector<messages::TaggedBlock> fork;
  std::vector<crypto::Ecc> fork_eccs{5};
  fork.push_back(tagged_blocks[3]);
  for (int i = 0; i < 5; i++) {
    auto &previous = fork[i];
    messages::TaggedBlock tagged_block;
    tooling::blockgen::blockgen_from_block(tagged_block.mutable_block(),
                                           previous.block(), 3 + i + 1);

    // Sadly the block has the same id as the block in the main branch
    // let's fix that
    tagged_block.mutable_block()
        ->mutable_coinbase()
        ->mutable_last_seen_block_id()
        ->CopyFrom(previous.block().header().id());
    auto output =
        tagged_block.mutable_block()->mutable_coinbase()->add_outputs();
    output->mutable_value()->set_value(i);
    output->mutable_key_pub()->CopyFrom(eccs[0].key_pub());
    messages::set_block_hash(tagged_block.mutable_block());

    auto branch_path = tagged_block.mutable_branch_path();
    branch_path->add_branch_ids(1);
    branch_path->add_branch_ids(0);
    branch_path->add_block_numbers(i);
    branch_path->add_block_numbers(3);
    branch_path->set_branch_id(1);
    branch_path->set_block_number(i);
    tagged_block.set_branch(messages::Branch::FORK);
    for (int j = 0; j <= i; j++) {
      auto balance = tagged_block.add_balances();
      balance->mutable_key_pub()->CopyFrom(fork_eccs[j].key_pub());
      balance->mutable_value()->set_value(17 + i);
      balance->set_enthalpy_begin("0");
      balance->set_enthalpy_end("0");
    }
    ASSERT_TRUE(ledger->insert_block(tagged_block));
    fork.push_back(tagged_block);
  }

  // Check the balances in the main branch
  for (int i = 0; i < 6; i++) {
    for (int j = 0; j < 5; j++) {
      auto balance = ledger->get_balance(eccs[j].key_pub(), tagged_blocks[i]);
      if (i > 0 && j < i) {
        ASSERT_EQ(balance.value().value(), i);
      } else {
        ASSERT_EQ(balance.value().value(), 0);
      }
      auto fork_balance =
          ledger->get_balance(fork_eccs[j].key_pub(), tagged_blocks[i]);
      ASSERT_EQ(fork_balance.value().value(), 0);
    }
  }

  // Check the balances in the fork branch
  for (int i = 0; i < 5; i++) {
    for (int j = 0; j < 5; j++) {
      auto balance = ledger->get_balance(eccs[j].key_pub(), fork[i + 1]);
      if (j < 3) {
        ASSERT_EQ(balance.value().value(), 3);
      } else {
        ASSERT_EQ(balance.value().value(), 0);
      }
      auto fork_balance =
          ledger->get_balance(fork_eccs[j].key_pub(), fork[i + 1]);
      if (j <= i) {
        ASSERT_EQ(fork_balance.value().value(), 17 + i);
      } else {
        ASSERT_EQ(fork_balance.value().value(), 0);
      }
    }
  }
}

TEST_F(LedgerMongodb, compute_new_balance) {
  // simple transaction, 2 block, 2 transaction, address0 send all it's ncc,
  // then in next block address 2 send half of it's one
  auto &key_pub0 = simulator.key_pubs[0];
  auto &key_pub1 = simulator.key_pubs[1];

  ASSERT_TRUE(simulator.consensus->add_transaction(
      ledger->send_ncc(simulator.keys[0].key_priv(), key_pub1, 1)));
  auto block = simulator.new_block();
  ASSERT_TRUE(simulator.consensus->add_block(block));
  messages::TaggedBlock tagged_block1;
  ledger->get_block(block.header().id(), &tagged_block1);

  auto balance0 = ledger->get_balance(key_pub0, tagged_block1);
  auto balance1 = ledger->get_balance(key_pub1, tagged_block1);
  auto balance_block1_address0 = balance0.value().value();
  auto balance_block1_address1 = balance1.value().value();
  auto enthalpy_begin_block1_address0 = std::stoi(balance0.enthalpy_begin());
  auto enthalpy_end_block1_address0 = std::stoi(balance0.enthalpy_end());
  auto enthalpy_begin_block1_address1 = std::stoi(balance1.enthalpy_begin());
  auto enthalpy_end_block1_address1 = std::stoi(balance1.enthalpy_end());

  ASSERT_TRUE(simulator.consensus->add_transaction(
      ledger->send_ncc(simulator.keys[1].key_priv(), key_pub0, 0.5)));
  block = simulator.new_block();
  ASSERT_TRUE(simulator.consensus->add_block(block));
  messages::TaggedBlock tagged_block2;
  ledger->get_block(block.header().id(), &tagged_block2);

  balance0 = ledger->get_balance(key_pub0, tagged_block2);
  balance1 = ledger->get_balance(key_pub1, tagged_block2);
  auto balance_block2_address0 = balance0.value().value();
  auto balance_block2_address1 = balance1.value().value();
  auto enthalpy_begin_block2_address0 = std::stoi(balance0.enthalpy_begin());
  auto enthalpy_end_block2_address0 = std::stoi(balance0.enthalpy_end());
  auto enthalpy_begin_block2_address1 = std::stoi(balance1.enthalpy_begin());
  auto enthalpy_end_block2_address1 = std::stoi(balance1.enthalpy_end());

  ASSERT_EQ(enthalpy_begin_block1_address0, ncc_block0.value());
  ASSERT_EQ(enthalpy_begin_block1_address1, ncc_block0.value());
  ASSERT_EQ(enthalpy_end_block1_address0, 0);  // we sent all our ncc
  ASSERT_EQ(enthalpy_end_block1_address1, enthalpy_begin_block1_address1);

  ASSERT_EQ(enthalpy_begin_block2_address0,
            enthalpy_end_block1_address0 + balance_block1_address0);
  ASSERT_EQ(enthalpy_begin_block2_address1,
            enthalpy_end_block1_address1 + balance_block1_address1);
  ASSERT_EQ(enthalpy_end_block2_address0, enthalpy_begin_block2_address0);
  ASSERT_EQ(enthalpy_end_block2_address1,
            (enthalpy_begin_block1_address1 + balance_block1_address1) *
                0.5);  // we sent 50% of all our ncc

  // try to spend more than we have (can be done if we receive more ncc in
  // the same block
  ASSERT_TRUE(ledger->add_to_transaction_pool(
      ledger->send_ncc(simulator.keys[0].key_priv(), key_pub0, 1.2)));
  double fee = 100;
  ASSERT_TRUE(simulator.consensus->add_transaction(ledger->send_ncc(
      simulator.keys[1].key_priv(), key_pub0, 0.3, messages::NCCAmount(fee))));
  block = simulator.new_block();
  ASSERT_TRUE(simulator.consensus->add_block(block));
  messages::TaggedBlock tagged_block3;
  ledger->get_block(block.header().id(), &tagged_block3);

  balance0 = ledger->get_balance(key_pub0, tagged_block3);
  balance1 = ledger->get_balance(key_pub1, tagged_block3);
  auto enthalpy_begin_block3_address0 = std::stoi(balance0.enthalpy_begin());
  auto enthalpy_end_block3_address0 = std::stoi(balance0.enthalpy_end());
  auto enthalpy_begin_block3_address1 = std::stoi(balance1.enthalpy_begin());
  auto enthalpy_end_block3_address1 = std::stoi(balance1.enthalpy_end());

  ASSERT_EQ(enthalpy_begin_block3_address0,
            enthalpy_end_block2_address0 + balance_block2_address0);
  ASSERT_EQ(enthalpy_begin_block3_address1,
            enthalpy_end_block2_address1 + balance_block2_address1);
  ASSERT_EQ(enthalpy_end_block3_address0,
            0);  // we sent more than 50% of all our ncc
  ASSERT_EQ(enthalpy_end_block3_address1,
            enthalpy_begin_block3_address1 *
                (0.7 - fee / balance_block2_address1));  // we sent 30% of all
                                                         // our ncc (+ fees)
}

}  // namespace tests
}  // namespace ledger
}  // namespace neuro
