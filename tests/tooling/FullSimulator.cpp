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
    }
    for (const auto &bot : simulator.bots) {
      LOG_DEBUG << "TOTORO" << bot->_config.networking().tcp().port() << " "
                << bot->_ledger->height() << std::endl;
    }
    for (const auto &bot : simulator.bots) {
      ASSERT_GT(bot->_ledger->height(), 0);
    }
  }
};

TEST_F(FullSimulator, full_simulator) { check_bots(); }

}  // namespace tests
}  // namespace tooling
}  // namespace neuro
