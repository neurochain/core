#include "tooling/Simulator.hpp"
#include <gtest/gtest.h>
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
  const messages::NCCSDF ncc_block0 = messages::ncc_amount(1000000);
  const messages::NCCSDF block_reward = messages::ncc_amount(100);
  const int nb_keys = 1000;

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
}

}  // namespace tests
}  // namespace tooling
}  // namespace neuro
