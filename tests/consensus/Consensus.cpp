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
  const messages::NCCAmount block_reward = messages::NCCAmount(100);
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
  void test_is_valid() {
    // Create 5 blocks with 7 transactions each
    simulator.run(3, 5);
    const auto tip = ledger->get_main_branch_tip();
    ASSERT_EQ(tip.block().header().height(), 3);

    for (int i = 1; i <= 3; i++) {
      messages::Block block;
      ledger->get_block(i, &block);
      for (int i = 1; i < block.transactions_size(); i++) {
        // Only test normal transactions and not coinbases
        messages::TaggedTransaction tagged_transaction;
        tagged_transaction.mutable_transaction()->CopyFrom(
            block.transactions(i));
        tagged_transaction.mutable_block_id()->CopyFrom(block.header().id());
        tagged_transaction.set_is_coinbase(false);
        ASSERT_TRUE(consensus.check_id(tagged_transaction));
        ASSERT_TRUE(consensus.check_signatures(tagged_transaction));
        ASSERT_TRUE(consensus.check_inputs(tagged_transaction));
        ASSERT_TRUE(consensus.check_outputs(tagged_transaction));
        ASSERT_TRUE(consensus.is_valid(tagged_transaction));
      }
    }

    // Coinbases should be invalid transactions because their inputs amounts do
    // not equal their outputs amounts
    messages::Block block0;
    ASSERT_TRUE(ledger->get_block(0, &block0));
    for (const auto &transaction : block0.transactions()) {
      messages::TaggedTransaction tagged_transaction;
      tagged_transaction.mutable_transaction()->CopyFrom(transaction);
      tagged_transaction.mutable_block_id()->CopyFrom(block0.header().id());
      tagged_transaction.set_is_coinbase(true);
      ASSERT_FALSE(consensus.check_outputs(tagged_transaction));
      ASSERT_FALSE(consensus.is_valid(tagged_transaction));
    }
  }
};

TEST_F(Consensus, is_valid) { test_is_valid(); }

}  // namespace tests
}  // namespace consensus
}  // namespace neuro
