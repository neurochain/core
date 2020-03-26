#include <gtest/gtest.h>

#include "ledger/LedgerMongodb.hpp"
#include "tooling/Simulator.hpp"

namespace neuro {
namespace tooling {
namespace tests {

class Simulator : public ::testing::Test {
 public:
  const std::string db_url = "mongodb://mongo:27017";
  const std::string db_name = "test_simulator";
  const messages::NCCAmount ncc_block0 = messages::NCCAmount(1000000);
  const int nb_keys = 20;

 protected:
  tooling::Simulator simulator;
  std::shared_ptr<neuro::ledger::LedgerMongodb> ledger;

  Simulator()
      : simulator(tooling::Simulator::Simulator::StaticSimulator(
            db_url, db_name, nb_keys, ncc_block0)),
        ledger(simulator.ledger) {}
};

/**
 * Test that a simulator can laod a block0 and have correct information
 */
TEST_F(Simulator, block0) {
  messages::Block block;
  ASSERT_TRUE(ledger->get_block(0, &block));
  ASSERT_EQ(ledger->height(), 0);
  ASSERT_EQ(block.coinbase().outputs_size(), nb_keys);
  ASSERT_EQ(ledger->total_nb_transactions(), 1);
}

/**
 * Test that created transaction have the correct recipient / sender / ncc value
 */
TEST_F(Simulator, send_ncc) {
  auto sender_private_key = simulator.keys[0].key_priv();
  auto recipient_public_key = simulator.key_pubs[1];
  auto transaction0 =
      ledger->send_ncc(sender_private_key, recipient_public_key, 0.5);
  ASSERT_EQ(transaction0.inputs_size(), 1);
  ASSERT_EQ(transaction0.outputs_size(), 1);
  ASSERT_EQ(transaction0.outputs(0).value().value(), ncc_block0.value() * 0.5);
  ledger->add_to_transaction_pool(transaction0);

  auto transaction1 =
      ledger->send_ncc(sender_private_key, recipient_public_key, 0.25);
  ASSERT_EQ(transaction1.inputs_size(), 1);
  ASSERT_EQ(transaction1.outputs(0).value().value(), ncc_block0.value() * 0.25);
  ledger->add_to_transaction_pool(transaction1);

  sender_private_key = simulator.keys[1].key_priv();
  recipient_public_key = simulator.key_pubs[1];
  auto transaction2 =
      ledger->send_ncc(sender_private_key, recipient_public_key, 0.1);
  ASSERT_EQ(transaction2.inputs_size(), 1);
  ASSERT_EQ(transaction2.outputs(0).value().value(), ncc_block0.value() * 0.1);
  ledger->add_to_transaction_pool(transaction2);
}

/**
 * Test that a making a random transaction is a valid one
 */
TEST_F(Simulator, random_transaction) {
  ASSERT_TRUE(ledger->add_to_transaction_pool(simulator.random_transaction()));
}

/**
 * Test that running the simulator create the correct amount of block and transaction
 */
TEST_F(Simulator, run) {
  // Create 10 block with 7 transactions each
  simulator.run(10, 7);
  ASSERT_EQ(ledger->height(), 10);
  for (int i = 1; i <= 10; i++) {
    messages::Block block;
    ASSERT_TRUE(ledger->get_block(i, &block));
    auto block_id = block.header().id();
    messages::set_block_hash(&block);
    ASSERT_EQ(block.header().id(), block_id);

    // Some send_ncc transactions are invalid
    // Because the balance does not use the transaction pool
    ASSERT_GT(block.transactions_size(), 3);
    ASSERT_LT(block.transactions_size(), 10);
    ASSERT_EQ(block.coinbase().outputs_size(), 1);
  }
}

}  // namespace tests
}  // namespace tooling
}  // namespace neuro
