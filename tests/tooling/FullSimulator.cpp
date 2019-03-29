#include "tooling/FullSimulator.hpp"
#include <gtest/gtest.h>

namespace neuro {
namespace tooling {
namespace tests {

class FullSimulator : public ::testing::Test {
 public:
  const std::string db_url = "mongodb://mongo:27017";
  const std::string db_name = "test_simulator";
  const int nb_bots = 10;
  const messages::NCCAmount ncc_block0 = messages::NCCAmount(1000000);

 protected:
  tooling::FullSimulator simulator;
  std::shared_ptr<neuro::ledger::LedgerMongodb> ledger;

  FullSimulator() : simulator(db_url, db_name, nb_bots, ncc_block0) {}
};

TEST_F(FullSimulator, full_simulator) { ASSERT_EQ(simulator.bots.size(), 10); }

}  // namespace tests
}  // namespace tooling
}  // namespace neuro
