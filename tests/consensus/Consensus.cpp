#include "consensus/Consensus.hpp"
#include <gtest/gtest.h>
#include <thread>
#include "ledger/LedgerMongodb.hpp"
#include "tooling/Simulator.hpp"

namespace neuro {
namespace consensus {
namespace tests {

class Consensus : public testing::Test {
 public:
  const std::string db_url = "mongodb://mongo:27017";
  const std::string db_name = "test_consensus";
  const messages::NCCAmount ncc_block0 = messages::NCCAmount(100000);
  const int nb_keys = 4;

 protected:
  tooling::Simulator simulator;
  std::shared_ptr<neuro::ledger::LedgerMongodb> ledger;
  std::shared_ptr<consensus::Consensus> consensus;

  Consensus()
      : simulator(tooling::Simulator::Simulator::StaticSimulator(
            db_url, db_name, nb_keys, ncc_block0)),
        ledger(simulator.ledger),
        consensus(simulator.consensus) {}

 public:
  void check_assembly_pii(const messages::Assembly &assembly) {
    ASSERT_TRUE(assembly.has_seed());
    ASSERT_TRUE(assembly.has_nb_key_pubs());
    ASSERT_TRUE(assembly.nb_key_pubs() >= 1);
    ASSERT_TRUE(assembly.finished_computation());
    for (int i = 0; i < assembly.nb_key_pubs(); i++) {
      messages::_KeyPub key_pub;
      ASSERT_TRUE(ledger->get_block_writer(assembly.id(), i, &key_pub));
    }
    std::vector<messages::Pii> piis;
    ledger->get_assembly_piis(assembly.id(), &piis);
    ASSERT_EQ(piis.size(), assembly.nb_key_pubs());
    for (const auto pii : piis) {
      ASSERT_GT(Double(pii.score()), 1);
    }
  }

  void test_is_valid_transaction() {
    // Create 3 blocks with 5 transactions each
    simulator.run(3, 5);
    const auto tip = ledger->get_main_branch_tip();
    ASSERT_EQ(tip.block().header().height(), 3);

    for (int i = 0; i <= 3; i++) {
      messages::Block block;
      ledger->get_block(i, &block);
      for (const auto &transaction : block.transactions()) {
        messages::TaggedTransaction tagged_transaction;
        tagged_transaction.mutable_transaction()->CopyFrom(transaction);
        tagged_transaction.mutable_block_id()->CopyFrom(block.header().id());
        tagged_transaction.set_is_coinbase(false);
        ASSERT_TRUE(consensus->check_id(tagged_transaction));
        ASSERT_TRUE(consensus->check_signatures(tagged_transaction));
        ASSERT_TRUE(consensus->check_double_inputs(tagged_transaction));
        ASSERT_TRUE(consensus->check_outputs(tagged_transaction));
        ASSERT_TRUE(consensus->is_valid(tagged_transaction));
      }
      messages::TaggedTransaction tagged_coinbase;
      tagged_coinbase.mutable_transaction()->CopyFrom(block.coinbase());
      tagged_coinbase.mutable_block_id()->CopyFrom(block.header().id());
      tagged_coinbase.set_is_coinbase(true);
      ASSERT_TRUE(consensus->check_id(tagged_coinbase));
      ASSERT_TRUE(consensus->check_coinbase(tagged_coinbase));
      ASSERT_TRUE(consensus->is_valid(tagged_coinbase));
    }
  }

  void test_is_valid_block() {
    // Create 3 blocks with 5 transactions each
    simulator.run(3, 5);
    messages::TaggedBlock tagged_block;
    for (int i = 0; i <= 3; i++) {
      ASSERT_TRUE(ledger->get_block(i, &tagged_block));
      ASSERT_TRUE(consensus->check_transactions_order(tagged_block));
      ASSERT_TRUE(consensus->check_block_id(tagged_block));
      ASSERT_TRUE(consensus->check_block_size(tagged_block));
      ASSERT_TRUE(consensus->check_block_timestamp(tagged_block));
      if (i == 0) {
        ASSERT_FALSE(consensus->is_valid(tagged_block));
      } else {
        ASSERT_TRUE(consensus->check_block_transactions(tagged_block));
        ASSERT_TRUE(consensus->check_block_height(tagged_block));
        ASSERT_TRUE(consensus->check_block_author(tagged_block));
        ASSERT_TRUE(consensus->is_valid(tagged_block));
      }
    }
  }

  void test_compute_assembly_pii() {
    std::vector<messages::Assembly> assemblies;
    bool compute_pii = false;
    simulator.run(consensus->config().blocks_per_assembly, 10, compute_pii);
    ASSERT_TRUE(ledger->get_assemblies_to_compute(&assemblies));
    ASSERT_EQ(assemblies.size(), 1);
    auto &assembly = assemblies[0];
    ASSERT_TRUE(consensus->compute_assembly_pii(assembly));
    ASSERT_TRUE(ledger->get_assembly(assembly.id(), &assembly));
    check_assembly_pii(assembly);
  }

  void test_start_computations() {
    std::vector<messages::Assembly> assemblies;
    ASSERT_FALSE(ledger->get_assemblies_to_compute(&assemblies));
    bool compute_pii = false;
    simulator.run(consensus->config().blocks_per_assembly, 10, compute_pii);
    ASSERT_TRUE(ledger->get_assemblies_to_compute(&assemblies));
    ASSERT_EQ(assemblies.size(), 1);
    auto assembly_id = assemblies[0].id();

    // Make sure that the compute_assembly thread has started the computation
    consensus->start_compute_pii_thread();
    std::this_thread::sleep_for(consensus->config().compute_pii_sleep);

    // Wait for the computation to be finished
    consensus->_stop_compute_pii = true;
    if (consensus->_compute_pii_thread.joinable()) {
      consensus->_compute_pii_thread.join();
    }
    ASSERT_FALSE(ledger->get_assemblies_to_compute(&assemblies));
    messages::Assembly assembly;
    ASSERT_TRUE(ledger->get_assembly(assembly_id, &assembly));
    ASSERT_TRUE(assembly.has_seed());
    ASSERT_TRUE(assembly.has_nb_key_pubs());
    ASSERT_TRUE(assembly.nb_key_pubs() >= 1);
    for (int i = 0; i < assembly.nb_key_pubs(); i++) {
      messages::_KeyPub key_pub;
      ASSERT_TRUE(ledger->get_block_writer(assembly_id, i, &key_pub));
    }
  }

  void test_build_block() {
    messages::TaggedBlock tagged_block;
    messages::Assembly assembly_minus_1, assembly_minus_2;
    bool include_transactions = false;
    ASSERT_TRUE(ledger->get_last_block(&tagged_block, include_transactions));
    ledger->get_assembly(tagged_block.previous_assembly_id(),
                         &assembly_minus_1);
    ledger->get_assembly(assembly_minus_1.previous_assembly_id(),
                         &assembly_minus_2);
    for (uint32_t i = 1; i < consensus->config().blocks_per_assembly; i++) {
      messages::_KeyPub key_pub;
      ASSERT_TRUE(consensus->get_block_writer(assembly_minus_2, i, &key_pub));
      int key_pub_index = -1;
      for (size_t i = 0; i < simulator.key_pubs.size(); i++) {
        if (key_pub == simulator.key_pubs[i]) {
          key_pub_index = i;
          break;
        }
      }
      ASSERT_NE(key_pub_index, -1);

      const auto &keys = simulator.keys[key_pub_index];
      messages::Block block;
      ASSERT_TRUE(consensus->build_block(keys, i, &block));
      ASSERT_EQ(block.header().height(), i);
      ASSERT_EQ(block.coinbase().outputs_size(), 1);
      ASSERT_EQ(block.header().has_id(), 1);
      ASSERT_EQ(block.header().has_author(), 1);
      messages::TaggedBlock tagged_block;
      tagged_block.mutable_block()->CopyFrom(block);

      // The timestamp of the block should be wrong we don't write it at the
      // correct time
      ASSERT_FALSE(consensus->check_block_timestamp(tagged_block));

      // So we fix it
      ASSERT_TRUE(ledger->get_block(block.header().previous_block_hash(),
                                    &tagged_block));
      block.mutable_header()->mutable_timestamp()->set_data(
          tagged_block.block().header().timestamp().data() +
          consensus->config().block_period);

      // Rewrite the block hash which should have changed
      messages::set_block_hash(&block);

      // Since we changed the hash we also need to change the signature
      crypto::sign(keys, &block);

      ASSERT_TRUE(consensus->add_block(block));
      std::vector<messages::TaggedBlock> blocks;
      ledger->get_blocks_by_previd(block.header().previous_block_hash(),
                                   &blocks);
      ASSERT_EQ(blocks.size(), 1);

      // Try inserting the same block again
      ASSERT_FALSE(consensus->add_block(block));
      blocks.clear();
      ledger->get_blocks_by_previd(block.header().previous_block_hash(),
                                   &blocks);
      ASSERT_EQ(blocks.size(), 1);
    }
  }

  uint64_t total_money() {
    uint64_t total = 0;
    for (const auto &key_pub : simulator.key_pubs) {
      total += ledger->balance(key_pub).value();
    }
    return total;
  }

  void test_money_supply() {
    ASSERT_EQ(total_money(), nb_keys * ncc_block0.value());
    for (int i = 0; i < 10; i++) {
      simulator.run(1, 5);
      ASSERT_EQ(
          total_money(),
          nb_keys * ncc_block0.value() +
              (i + 1) * simulator.consensus->config().block_reward.value());
    }
  }
};

TEST_F(Consensus, is_valid_transaction) { test_is_valid_transaction(); }

TEST_F(Consensus, is_valid_block) { test_is_valid_block(); }

TEST_F(Consensus, get_current_height) {
  messages::Block block0;
  ASSERT_TRUE(ledger->get_block(0, &block0));
  auto height = consensus->get_current_height();
  ASSERT_TRUE(
      abs(height - ((std::time(nullptr) - block0.header().timestamp().data()) /
                    consensus->config().block_period)) <= 1);
}

TEST_F(Consensus, add_block) {
  messages::TaggedBlock block0;
  ASSERT_TRUE(ledger->get_block(0, &block0));
  auto block = simulator.new_block(10, block0);
  ASSERT_TRUE(consensus->add_block(block));
}

TEST_F(Consensus, compute_assembly_pii) { test_compute_assembly_pii(); }

TEST_F(Consensus, start_computations) { test_start_computations(); }

TEST_F(Consensus, add_transaction) {
  auto t0 = ledger->send_ncc(simulator.keys[0].key_priv(),
                             simulator.key_pubs[1], 0.5);
  ASSERT_TRUE(consensus->add_transaction(t0));

  // The second time the transaction should be not be valid because it already
  // exists
  ASSERT_FALSE(consensus->add_transaction(t0));

  auto t1 = ledger->send_ncc(simulator.keys[0].key_priv(),
                             simulator.key_pubs[0], 0.5);
  ASSERT_TRUE(consensus->add_transaction(t1));
}

TEST_F(Consensus, build_block) { test_build_block(); }

TEST_F(Consensus, add_denunciations) {
  // Let's make the first miner double mine
  auto block1 = simulator.new_block();
  auto block1_bis = simulator.new_block(1);
  LOG_DEBUG << "BLOCK1 ID " << block1.header().id();
  LOG_DEBUG << "BLOCK1 NB TRANSACTIONS " << block1.transactions_size();
  LOG_DEBUG << "BLOCK1 BIS ID " << block1_bis.header().id();
  LOG_DEBUG << "BLOCK1 BIS NB TRANSACTIONS " << block1_bis.transactions_size();
  ASSERT_TRUE(simulator.consensus->add_block(block1));
  ASSERT_TRUE(simulator.consensus->add_block(block1_bis));
  messages::TaggedBlock tagged_block1;
  bool include_transactions = false;
  ASSERT_TRUE(ledger->get_last_block(&tagged_block1, include_transactions));
  ASSERT_EQ(tagged_block1.block().header().id(), block1.header().id());
  auto block = simulator.new_block();
  ASSERT_EQ(block.denunciations_size(), 1);
  auto denunciation = block.denunciations(0);
  ASSERT_EQ(denunciation.block_id(), block1_bis.header().id());
  ASSERT_EQ(denunciation.block_author(), block1_bis.header().author());
  ASSERT_EQ(denunciation.block_height(), block1_bis.header().height());
}

TEST_F(Consensus, money_supply) { test_money_supply(); }

}  // namespace tests
}  // namespace consensus
}  // namespace neuro
