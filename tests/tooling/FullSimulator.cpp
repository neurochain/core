#include "tooling/FullSimulator.hpp"
#include <gtest/gtest.h>

namespace neuro {
namespace tooling {
namespace tests {

class FullSimulator : public ::testing::Test {
 public:
  const std::string db_url = "mongodb://mongo:27017";
  const std::string db_name = "test_simulator";
  const int nb_bots = 4;
  const messages::NCCAmount ncc_block0 = messages::NCCAmount(1000000);

 protected:
  tooling::FullSimulator simulator;
  std::shared_ptr<neuro::ledger::LedgerMongodb> ledger;

  FullSimulator() : simulator(db_url, db_name, nb_bots, ncc_block0) {}

  void check_bots() {
    ASSERT_EQ(simulator.bots.size(), nb_bots);
    std::this_thread::sleep_for(30s);
    for (const auto &bot : simulator.bots) {
      ASSERT_GT(bot->connected_peers().size(), 2);
      auto height = bot->_ledger->height();
      ASSERT_GT(height, 20);
      ASSERT_LT(height, 30);
      auto total_nb_blocks = bot->_ledger->total_nb_blocks();
      ASSERT_GT(total_nb_blocks, 20);
      ASSERT_LT(total_nb_blocks, 30);
      auto total_nb_transactions = bot->_ledger->total_nb_transactions();
      ASSERT_GT(total_nb_transactions, 30);
    }
  }
};

TEST_F(FullSimulator, full_simulator) { check_bots(); }

}  // namespace tests
}  // namespace tooling
}  // namespace neuro
