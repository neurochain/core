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
    ASSERT_TRUE(ledger->insert_block(&block1));
    ASSERT_TRUE(ledger->insert_block(&block2));

    // Insert the fork branch
    ASSERT_TRUE(ledger->insert_block(&fork1));
    ASSERT_TRUE(ledger->insert_block(&fork2));

    ledger->update_branch_tag(block0.header().id(), messages::Branch::MAIN);
    ASSERT_TRUE(ledger->set_block_verified(block1.header().id(), 2,
                                           block0.header().id()));
    ASSERT_TRUE(ledger->set_block_verified(block2.header().id(), 3,
                                           block0.header().id()));
    ASSERT_TRUE(ledger->set_block_verified(fork1.header().id(), 2,
                                           block0.header().id()));
    ASSERT_TRUE(ledger->set_block_verified(fork2.header().id(), 2.5,
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
    ASSERT_TRUE(ledger->insert_block(&fork3));
    ASSERT_TRUE(ledger->set_block_verified(fork3.header().id(), 3.5,
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

  ASSERT_EQ(nb_keys, ledger->total_nb_transactions());
  ASSERT_EQ(0, block0.transactions().size());
  ASSERT_EQ(nb_keys, block0.coinbases().size());
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
  ASSERT_EQ(tagged_block.branch(), messages::Branch::UNVERIFIED);
  messages::BranchPath branch_path;
  branch_path.add_branch_ids(0);
  branch_path.add_block_numbers(1);
  ASSERT_EQ(tagged_block.branch_path(), branch_path);
  tagged_block.Clear();
  ASSERT_TRUE(ledger->get_block(block2.header().id(), &tagged_block));
  ASSERT_EQ(tagged_block.branch(), messages::Branch::UNVERIFIED);
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
  ASSERT_EQ(tagged_block.branch(), messages::Branch::UNVERIFIED);
  messages::BranchPath branch_path;
  branch_path.add_branch_ids(0);
  branch_path.add_block_numbers(1);
  ASSERT_EQ(tagged_block.branch_path(), branch_path);
  tagged_block.Clear();
  ASSERT_TRUE(ledger->get_block(block2.header().id(), &tagged_block));
  ASSERT_EQ(tagged_block.branch(), messages::Branch::UNVERIFIED);
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
  ASSERT_EQ(tagged_block.branch(), messages::Branch::UNVERIFIED);
  messages::BranchPath branch_path;
  branch_path.add_branch_ids(0);
  branch_path.add_block_numbers(2);
  ASSERT_EQ(tagged_block.branch_path(), branch_path);
  tagged_block.Clear();
  ASSERT_TRUE(ledger->get_block(fork2.header().id(), &tagged_block));
  ASSERT_EQ(tagged_block.branch(), messages::Branch::UNVERIFIED);
  branch_path.Clear();
  branch_path.add_branch_ids(1);
  branch_path.add_branch_ids(0);
  branch_path.add_block_numbers(1);
  branch_path.add_block_numbers(0);
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
  ASSERT_TRUE(ledger->insert_block(&block2));
  messages::TaggedBlock tagged_block;
  ASSERT_TRUE(ledger->get_block(block2.header().id(), &tagged_block));
  ASSERT_EQ(tagged_block.branch(), messages::Branch::DETACHED);
  std::vector<messages::TaggedBlock> unscored_forks;
  ASSERT_TRUE(ledger->get_unverified_blocks(&unscored_forks));
  ASSERT_EQ(unscored_forks.size(), 0);

  // Insert the block1 which should attach the block2
  ASSERT_TRUE(ledger->insert_block(&block1));
  tagged_block.Clear();
  ASSERT_TRUE(ledger->get_block(block1.header().id(), &tagged_block));
  ASSERT_EQ(tagged_block.branch(), messages::Branch::UNVERIFIED);
  ASSERT_TRUE(ledger->get_block(block2.header().id(), &tagged_block));
  ASSERT_EQ(tagged_block.branch(), messages::Branch::UNVERIFIED);
  unscored_forks.clear();
  ASSERT_TRUE(ledger->get_unverified_blocks(&unscored_forks));
  ASSERT_EQ(unscored_forks.size(), 2);

  ASSERT_TRUE(ledger->set_block_verified(block1.header().id(), 1.17,
                                         block0.header().id()));
  unscored_forks.clear();
  ASSERT_TRUE(ledger->get_unverified_blocks(&unscored_forks));
  ASSERT_EQ(unscored_forks.size(), 1);

  ASSERT_TRUE(ledger->set_block_verified(block2.header().id(), 2.17,
                                         block0.header().id()));
  unscored_forks.clear();
  ASSERT_TRUE(ledger->get_unverified_blocks(&unscored_forks));
  ASSERT_EQ(unscored_forks.size(), 0);
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

  // set_nb_addresses
  ASSERT_FALSE(assembly.has_nb_addresses());
  ASSERT_TRUE(ledger->set_nb_addresses(assembly.id(), 17));
  ASSERT_TRUE(
      ledger->get_assembly(tagged_block.block().header().id(), &assembly));
  ASSERT_EQ(assembly.nb_addresses(), 17);

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
    pii.mutable_address()->CopyFrom(messages::Address(keys[i].public_key()));
    pii.mutable_assembly_id()->CopyFrom(assembly_id);
    pii.set_score(10 - i);
    pii.set_rank(i);
    ASSERT_TRUE(ledger->set_pii(pii));
  }
  for (int i = 0; i < 5; i++) {
    messages::Address address;
    ASSERT_TRUE(ledger->get_block_writer(assembly_id, i, &address));
    ASSERT_EQ(address, messages::Address(keys[i].public_key()));
  }
}

TEST_F(LedgerMongodb, integrity) {
  messages::Integrity integrity;
  integrity.mutable_address()->CopyFrom(messages::Address::random());

  // It does not really matter yet I need a hash for the assembly id
  messages::TaggedBlock block0;
  ASSERT_TRUE(ledger->get_last_block(&block0));
  ASSERT_EQ(block0.block().header().height(), 0);
  integrity.mutable_assembly_id()->CopyFrom(block0.block().header().id());
  integrity.set_assembly_height(0);
  integrity.set_score(17);
  integrity.mutable_branch_path()->CopyFrom(block0.branch_path());
  ASSERT_TRUE(ledger->set_integrity(integrity));

  auto assembly_height = 0;
  auto integrity_score = ledger->get_integrity(
      integrity.address(), assembly_height, block0.branch_path());
  ASSERT_EQ(integrity_score, 17);

  auto block = simulator.new_block();
  simulator.consensus->add_block(&block);
  messages::TaggedBlock block1;
  ASSERT_TRUE(ledger->get_last_block(&block1));
  ASSERT_EQ(block1.block().header().height(), 1);

  assembly_height = 1;
  integrity_score = ledger->get_integrity(integrity.address(), assembly_height,
                                          block1.branch_path());
  ASSERT_EQ(integrity_score, 17);

  integrity.set_assembly_height(1);
  integrity.set_score(18);
  integrity.mutable_branch_path()->CopyFrom(block1.branch_path());
  ASSERT_TRUE(ledger->set_integrity(integrity));

  // We should get the new integrity when looking at this assembly height
  assembly_height = 1;
  integrity_score = ledger->get_integrity(integrity.address(), assembly_height,
                                          block1.branch_path());
  ASSERT_EQ(integrity_score, 18);

  // We should get the old integrity when looking at this assembly height
  assembly_height = 0;
  integrity_score = ledger->get_integrity(integrity.address(), assembly_height,
                                          block0.branch_path());
  ASSERT_EQ(integrity_score, 17);

  // And if we look in a different branch path that forked at block0 we should
  // get the old integrity
  messages::BranchPath branch_path;
  branch_path.add_branch_ids(1);
  branch_path.add_block_numbers(0);
  branch_path.add_branch_ids(0);
  branch_path.add_block_numbers(0);
  assembly_height = 1;
  integrity_score =
      ledger->get_integrity(integrity.address(), assembly_height, branch_path);
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
  auto &address = simulator.addresses[0];
  auto transactions = ledger->list_transactions(address).transactions();
  ASSERT_EQ(transactions.size(), 1);
  ASSERT_EQ(transactions[0].outputs(0).address(), address);

  // Check that we only get incoming transactions
  // Send everything so that there is no change output going back to the address
  auto transaction = ledger->send_ncc(simulator.keys[0].private_key(),
                                      simulator.addresses[1], 1);
  simulator.consensus->add_transaction(transaction);
  transactions = ledger->list_transactions(address).transactions();
  ASSERT_EQ(transactions.size(), 1);

  // Check that we do get transactions from the transaction pool
  transaction = ledger->send_ncc(simulator.keys[1].private_key(),
                                 simulator.addresses[0], 0.5);
  simulator.consensus->add_transaction(transaction);
  transactions = ledger->list_transactions(address).transactions();
  ASSERT_EQ(transactions.size(), 2);

  // Putting the transactions in a block should not change anything
  auto block = simulator.new_block();
  bool has_coinbase = block.coinbases(0).outputs(0).address() == address;
  simulator.consensus->add_block(&block);
  transactions = ledger->list_transactions(address).transactions();
  ASSERT_EQ(transactions.size(), has_coinbase ? 3 : 2);
}

TEST_F(LedgerMongodb, is_unspent_output) {
  messages::TaggedBlock block0;
  ASSERT_TRUE(ledger->get_last_block(&block0));
  auto coinbase =
      block0.block().coinbases(0).outputs(0).address() == simulator.addresses[0]
          ? block0.block().coinbases(0)
          : block0.block().coinbases(1);
  ASSERT_TRUE(ledger->is_unspent_output(coinbase.id(), 0));

  // Spend the output
  auto transaction = ledger->send_ncc(simulator.keys[0].private_key(),
                                      simulator.addresses[1], 0.5);
  simulator.consensus->add_transaction(transaction);
  ASSERT_FALSE(ledger->is_unspent_output(coinbase.id(), 0));

  // But it is not spent if I don't consider the transaction pool
  bool include_transaction_pool = false;
  auto &tip = block0;
  ASSERT_TRUE(ledger->is_unspent_output(coinbase.id(), 0, tip,
                                        include_transaction_pool));

  // Let's add a block with the transaction
  auto new_block = simulator.new_block();
  simulator.consensus->add_block(&new_block);
  include_transaction_pool = false;
  ASSERT_TRUE(ledger->is_unspent_output(coinbase.id(), 0, block0,
                                        include_transaction_pool));
  include_transaction_pool = true;
  ASSERT_TRUE(ledger->is_unspent_output(coinbase.id(), 0, block0,
                                        include_transaction_pool));
  ASSERT_FALSE(ledger->is_unspent_output(coinbase.id(), 0));
}

TEST_F(LedgerMongodb, get_outputs_for_address) {
  auto transaction = ledger->send_ncc(simulator.keys[0].private_key(),
                                      simulator.addresses[1], 0.5);
  simulator.consensus->add_transaction(transaction);
  auto outputs =
      ledger->get_outputs_for_address(transaction.id(), simulator.addresses[1]);
  ASSERT_EQ(outputs.size(), 1);
  ASSERT_EQ(outputs.at(0).output_id(), 0);
  ASSERT_EQ(outputs.at(0).address(), simulator.addresses[1]);
  outputs =
      ledger->get_outputs_for_address(transaction.id(), simulator.addresses[0]);
  ASSERT_EQ(outputs.size(), 1);
  ASSERT_EQ(outputs.at(0).output_id(), 1);
  ASSERT_EQ(outputs.at(0).address(), simulator.addresses[0]);
}

TEST_F(LedgerMongodb, has_received_transaction) {
  ASSERT_TRUE(ledger->has_received_transaction(simulator.addresses[0]));
  auto address = messages::Address::random();
  ASSERT_FALSE(ledger->has_received_transaction(address));
  auto transaction =
      ledger->send_ncc(simulator.keys[0].private_key(), address, 0.5);
  simulator.consensus->add_transaction(transaction);
  ASSERT_TRUE(ledger->has_received_transaction(address));
}

TEST_F(LedgerMongodb, get_last_blocks) {
  for (int i = 0; i < 2; i++) {
    auto new_block = simulator.new_block();
    simulator.consensus->add_block(&new_block);
  }
  auto blocks = ledger->get_last_blocks(2);
  ASSERT_EQ(blocks.size(), 2);
  ASSERT_EQ(blocks.at(0).header().height(), 2);
  ASSERT_EQ(blocks.at(1).header().height(), 1);
}

TEST_F(LedgerMongodb, list_unspent_transactions) {
  auto &address = simulator.addresses[0];
  auto transactions = ledger->list_unspent_transactions(address);
  ASSERT_EQ(transactions.size(), 1);

  // Send everything so that there is no change output going back to the
  // address
  auto transaction = ledger->send_ncc(simulator.keys[0].private_key(),
                                      simulator.addresses[1], 1);
  ASSERT_EQ(transaction.outputs_size(), 1);
  simulator.consensus->add_transaction(transaction);
  transactions = ledger->list_unspent_transactions(address);
  ASSERT_EQ(transactions.size(), 0);
  transactions = ledger->list_unspent_transactions(simulator.addresses[1]);
  ASSERT_EQ(transactions.size(), 2);

  // Send back the money
  transaction = ledger->send_ncc(simulator.keys[1].private_key(),
                                 simulator.addresses[0], 0.5);
  ASSERT_EQ(transaction.outputs_size(), 2);
  simulator.consensus->add_transaction(transaction);
  transactions = ledger->list_unspent_transactions(address);
  ASSERT_EQ(transactions.size(), 1);
  ASSERT_EQ(transactions.at(0).transaction_id(), transaction.id());
  transactions = ledger->list_unspent_transactions(simulator.addresses[1]);
  ASSERT_EQ(transactions.size(), 1);
  ASSERT_EQ(transactions.at(0).transaction_id(), transaction.id());

  // Putting the transactions in a block should not change anything
  auto block = simulator.new_block();
  bool has_coinbase = block.coinbases(0).outputs(0).address() == address;
  simulator.consensus->add_block(&block);
  transactions = ledger->list_unspent_transactions(address);
  ASSERT_EQ(transactions.size(), has_coinbase ? 2 : 1);
}

TEST_F(LedgerMongodb, list_unspent_outputs) {
  auto &address0 = simulator.addresses[0];
  auto &address1 = simulator.addresses[1];
  auto transactions = ledger->list_unspent_outputs(address0);
  ASSERT_EQ(transactions.size(), 1);
  ASSERT_EQ(transactions.at(0).outputs_size(), 1);

  // Send everything so that there is no change output going back to the
  // address
  auto transaction =
      ledger->send_ncc(simulator.keys[0].private_key(), address1, 1);
  ASSERT_EQ(transaction.outputs_size(), 1);
  simulator.consensus->add_transaction(transaction);
  transactions = ledger->list_unspent_outputs(address0);
  ASSERT_EQ(transactions.size(), 0);
  transactions = ledger->list_unspent_outputs(address1);
  ASSERT_EQ(transactions.size(), 2);
  ASSERT_EQ(transactions.at(0).outputs_size(), 1);
  ASSERT_EQ(transactions.at(1).outputs_size(), 1);

  // Send back the money
  transaction = ledger->send_ncc(simulator.keys[1].private_key(),
                                 simulator.addresses[0], 0.5);
  ASSERT_EQ(transaction.outputs_size(), 2);
  simulator.consensus->add_transaction(transaction);
  for (const auto &address : {address0, address1}) {
    transactions = ledger->list_unspent_outputs(address);
    ASSERT_EQ(transactions.size(), 1);
    ASSERT_EQ(transactions.at(0).id(), transaction.id());
    ASSERT_EQ(transactions.at(0).outputs_size(), 1);
  }

  // Putting the transactions in a block should not change anything
  auto block = simulator.new_block();
  simulator.consensus->add_block(&block);
  for (const auto &address : {address0, address1}) {
    bool has_coinbase = block.coinbases(0).outputs(0).address() == address;
    transactions = ledger->list_unspent_outputs(address);
    ASSERT_EQ(transactions.size(), has_coinbase ? 2 : 1);
  }
}

TEST_F(LedgerMongodb, balance) {
  auto &address0 = simulator.addresses[0];
  auto &address1 = simulator.addresses[1];
  ASSERT_EQ(ledger->balance(address0), ncc_block0);
  ASSERT_EQ(ledger->balance(address1), ncc_block0);
  simulator.consensus->add_transaction(
      ledger->send_ncc(simulator.keys[0].private_key(), address1, 1));
  ASSERT_EQ(ledger->balance(address0).value(), 0);
  ASSERT_EQ(ledger->balance(address1).value(), 2 * ncc_block0.value());
  simulator.consensus->add_transaction(
      ledger->send_ncc(simulator.keys[1].private_key(), address0, 0.5));
  ASSERT_EQ(ledger->balance(address0), ncc_block0);
  ASSERT_EQ(ledger->balance(address1), ncc_block0);
}

}  // namespace tests
}  // namespace ledger
}  // namespace neuro
