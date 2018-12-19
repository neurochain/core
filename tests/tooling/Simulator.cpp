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

std::shared_ptr<ledger::LedgerMongodb> empty_ledger(
    messages::config::Database config) {
  tooling::blockgen::create_empty_db(config);
  return std::make_shared<neuro::ledger::LedgerMongodb>(config);
}

class Simulator : public ::testing::Test {
 public:
  const std::string url = "mongodb://mongo:27017";
  const std::string db_name = "test_simulator";
  const messages::NCCAmount ncc_block0 = messages::ncc_amount(1000000);
  const messages::NCCAmount block_reward = messages::ncc_amount(100);
  const int nb_keys = 20;

 protected:
  tooling::Simulator simulator;
  std::shared_ptr<neuro::ledger::LedgerMongodb> ledger;

  Simulator()
      : simulator(url, db_name, nb_keys, ncc_block0, block_reward),
        ledger(simulator.ledger) {}
};

TEST_F(Simulator, block0) {
  messages::Block block;
  ASSERT_TRUE(ledger->get_block(0, &block));
  ASSERT_EQ(ledger->height(), 0);
  ASSERT_EQ(block.transactions_size(), nb_keys);
  ASSERT_EQ(ledger->total_nb_transactions(), nb_keys);
}

TEST_F(Simulator, send_ncc) {
  auto sender_private_key = simulator.keys[0].private_key();
  auto recipient_public_key = simulator.addresses[1];
  auto transaction0 =
      simulator.send_ncc(sender_private_key, recipient_public_key, 0.5);
  ledger->add_to_transaction_pool(transaction0);
  auto transaction1 =
      simulator.send_ncc(sender_private_key, recipient_public_key, 0.5);
  ASSERT_EQ(transaction1.inputs(0).id(), transaction0.id());
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

    // 7 transaction + coinbase
    ASSERT_EQ(block.transactions_size(), 8);
  }
}

}  // namespace tests
}  // namespace tooling
}  // namespace neuro
