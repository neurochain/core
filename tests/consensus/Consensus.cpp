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
  const messages::NCCAmount block_reward = consensus::BLOCK_REWARD;
  const int nb_keys = 2;

 protected:
  tooling::Simulator simulator;
  std::shared_ptr<neuro::ledger::LedgerMongodb> ledger;
  consensus::Consensus consensus;

  Consensus()
      : simulator(tooling::Simulator(db_url, db_name, nb_keys, ncc_block0,
                                     block_reward)),
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
      auto block = tagged_block.mutable_block();
      ASSERT_TRUE(consensus.check_transactions_order(*block));
      ASSERT_TRUE(consensus.check_block_id(block));
      ASSERT_TRUE(consensus.check_block_size(*block));
      if (i == 0) {
        ASSERT_FALSE(consensus.is_valid(block));
      } else {
        ASSERT_TRUE(consensus.check_block_transactions(*block));
        ASSERT_TRUE(consensus.check_block_author(*block));
        ASSERT_TRUE(consensus.check_block_height(*block));
        ASSERT_TRUE(consensus.is_valid(block));
      }
    }
  }
};

TEST_F(Consensus, is_valid_transaction) { test_is_valid_transaction(); }

// TODO separate tests for each transaction check

TEST_F(Consensus, is_valid_block) { test_is_valid_block(); }

}  // namespace tests
}  // namespace consensus
}  // namespace neuro
