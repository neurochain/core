#include "tooling/Simulator.hpp"
#include <gtest/gtest.h>
#include <boost/stacktrace.hpp>
#include "common/types.hpp"
#include "ledger/LedgerMongodb.hpp"
#include "messages/Message.hpp"
#include "messages/config/Config.hpp"
#include "messages/config/Database.hpp"
#include "tooling/blockgen.hpp"

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

TEST_F(Simulator, block0) {
  messages::Block block;
  ASSERT_TRUE(ledger->get_block(0, &block));
  ASSERT_EQ(ledger->height(), 0);
  ASSERT_EQ(block.coinbase().outputs_size(), nb_keys);
  ASSERT_EQ(ledger->total_nb_transactions(), 1);
}

TEST_F(Simulator, send_ncc) {
  auto sender_private_key = simulator.keys[0].key_priv();
  auto recipient_public_key = simulator.addresses[1];
  auto transaction0 =
      ledger->send_ncc(sender_private_key, recipient_public_key, 0.5);
  ASSERT_EQ(transaction0.inputs_size(), 1);
  ASSERT_EQ(transaction0.outputs_size(), 2);
  ASSERT_EQ(transaction0.outputs(0).value().value(), ncc_block0.value() * 0.5);
  ASSERT_EQ(transaction0.outputs(1).value().value(), ncc_block0.value() * 0.5);
  ledger->add_to_transaction_pool(transaction0);

  auto transaction1 =
      ledger->send_ncc(sender_private_key, recipient_public_key, 0.5);
  ASSERT_EQ(transaction1.inputs(0).id(), transaction0.id());
  ASSERT_EQ(transaction1.inputs_size(), 1);
  ASSERT_EQ(transaction1.outputs_size(), 2);
  ASSERT_EQ(transaction1.outputs(0).value().value(), ncc_block0.value() * 0.25);
  ASSERT_EQ(transaction1.outputs(1).value().value(), ncc_block0.value() * 0.25);
  ledger->add_to_transaction_pool(transaction1);

  sender_private_key = simulator.keys[1].key_priv();
  recipient_public_key = simulator.addresses[1];
  auto transaction2 =
      ledger->send_ncc(sender_private_key, recipient_public_key, 0.5);
  ASSERT_EQ(transaction2.inputs_size(), 3);
  ASSERT_EQ(transaction2.outputs_size(), 2);
  ASSERT_EQ(transaction2.outputs(0).value().value(),
            ncc_block0.value() * 0.875);
  ASSERT_EQ(transaction2.outputs(1).value().value(),
            ncc_block0.value() * 0.875);
  ledger->add_to_transaction_pool(transaction2);

  sender_private_key = simulator.keys[1].key_priv();
  recipient_public_key = simulator.addresses[3];
  auto transaction3 =
      ledger->send_ncc(sender_private_key, recipient_public_key, 0.5);
  ASSERT_EQ(transaction3.inputs_size(), 2);
  ASSERT_EQ(transaction3.outputs_size(), 2);
  ASSERT_EQ(transaction3.outputs(0).value().value(),
            ncc_block0.value() * 0.875);
  ASSERT_EQ(transaction3.outputs(1).value().value(),
            ncc_block0.value() * 0.875);
}

TEST_F(Simulator, random_transaction) {
  ASSERT_TRUE(ledger->add_to_transaction_pool(simulator.random_transaction()));
}

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

    ASSERT_EQ(block.transactions_size(), 7);
    ASSERT_EQ(block.coinbase().outputs_size(), 1);
  }
}

}  // namespace tests
}  // namespace tooling
}  // namespace neuro
