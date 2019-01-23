#include "consensus/Consensus.hpp"
#include <gtest/gtest.h>
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
  const int nb_keys = 2;

 protected:
  consensus::Config consensus_config;
  tooling::Simulator simulator;
  std::shared_ptr<neuro::ledger::LedgerMongodb> ledger;
  consensus::Consensus consensus;

  Consensus()
      : consensus_config(),
        simulator(tooling::Simulator(db_url, db_name, nb_keys, ncc_block0,
                                     consensus_config.block_reward)),
        ledger(simulator.ledger),
        consensus(consensus::Consensus(ledger)) {}

 public:
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
        ASSERT_TRUE(consensus.check_id(tagged_transaction));
        ASSERT_TRUE(consensus.check_signatures(tagged_transaction));
        ASSERT_TRUE(consensus.check_inputs(tagged_transaction));
        ASSERT_TRUE(consensus.check_double_inputs(tagged_transaction));
        ASSERT_TRUE(consensus.check_outputs(tagged_transaction));
        ASSERT_TRUE(consensus.is_valid(tagged_transaction));
      }
      for (const auto &transaction : block.coinbases()) {
        messages::TaggedTransaction tagged_transaction;
        tagged_transaction.mutable_transaction()->CopyFrom(transaction);
        tagged_transaction.mutable_block_id()->CopyFrom(block.header().id());
        tagged_transaction.set_is_coinbase(true);
        ASSERT_TRUE(consensus.check_id(tagged_transaction));
        ASSERT_TRUE(consensus.check_coinbase(tagged_transaction));
        ASSERT_TRUE(consensus.is_valid(tagged_transaction));
      }
    }
  }

  void test_is_valid_block() {
    // Create 3 blocks with 5 transactions each
    simulator.run(3, 5);
    messages::TaggedBlock tagged_block;
    for (int i = 0; i <= 3; i++) {
      ASSERT_TRUE(ledger->get_block(i, &tagged_block));
      ASSERT_TRUE(consensus.check_transactions_order(tagged_block));
      ASSERT_TRUE(consensus.check_block_id(&tagged_block));
      ASSERT_TRUE(consensus.check_block_size(tagged_block));
      ASSERT_TRUE(consensus.check_block_timestamp(tagged_block));
      if (i == 0) {
        ASSERT_FALSE(consensus.is_valid(&tagged_block));
      } else {
        ASSERT_TRUE(consensus.check_block_transactions(tagged_block));
        ASSERT_TRUE(consensus.check_block_height(tagged_block));
        ASSERT_TRUE(consensus.check_block_author(tagged_block));
        ASSERT_TRUE(consensus.is_valid(&tagged_block));
      }
    }
  }
};

TEST_F(Consensus, is_valid_transaction) { test_is_valid_transaction(); }

TEST_F(Consensus, is_valid_block) { test_is_valid_block(); }

TEST_F(Consensus, get_current_height) {
  messages::Block block0;
  ASSERT_TRUE(ledger->get_block(0, &block0));
  auto height = consensus.get_current_height();
  ASSERT_TRUE(
      abs(height - ((std::time(nullptr) - block0.header().timestamp().data()) /
                    consensus.config.block_period)) <= 1);
}

TEST_F(Consensus, add_block) {
  messages::TaggedBlock block0;
  ASSERT_TRUE(ledger->get_block(0, &block0));
  auto block = simulator.new_block(10, block0);
  ASSERT_TRUE(consensus.add_block(&block));
}

TEST_F(Consensus, start_computations) {
  std::vector<messages::Assembly> assemblies;
  ASSERT_FALSE(ledger->get_assemblies_to_compute(&assemblies));
  simulator.run(consensus.config.blocks_per_assembly, 10);
  ASSERT_TRUE(ledger->get_assemblies_to_compute(&assemblies));
  ASSERT_EQ(assemblies.size(), 1);
}

}  // namespace tests
}  // namespace consensus
}  // namespace neuro
