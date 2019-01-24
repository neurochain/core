#include "consensus/Pii.hpp"
#include <gtest/gtest.h>
#include "ledger/LedgerMongodb.hpp"
#include "tooling/Simulator.hpp"

namespace neuro {
namespace consensus {
namespace tests {

class Pii : public testing::Test {
 public:
  const std::string db_url = "mongodb://mongo:27017";
  const std::string db_name = "test_pii";
  const messages::NCCAmount ncc_block0 = messages::NCCAmount(1000000);
  const int nb_keys = 20;
  tooling::Simulator simulator;
  std::shared_ptr<neuro::ledger::LedgerMongodb> ledger;
  consensus::Pii pii;

  Pii()
      : simulator(tooling::Simulator(db_url, db_name, nb_keys, ncc_block0)),
        ledger(simulator.ledger),
        pii(ledger, simulator.consensus->config) {}
};

TEST(Addresses, get_pii) {
  Addresses addresses;
  auto a0 = messages::Address::random();
  auto a1 = messages::Address::random();
  auto a2 = messages::Address::random();
  addresses.add_enthalpy(a0, a1, 100);

  // Entropy is 0 when there is only 1 transaction and floored to 1
  ASSERT_EQ(addresses.get_entropy(a0), 1);

  // Entropy is 100 because -0.5 * log2(0.5) == 1
  addresses.add_enthalpy(a0, a2, 100);

  ASSERT_EQ(addresses.get_entropy(a0), 100);
  ASSERT_EQ(addresses.get_entropy(a1), 1);
  ASSERT_EQ(addresses.get_entropy(a2), 1);
  addresses.add_enthalpy(a1, a2, 100);

  // a2 should have an outgoing entropy of 0 and incoming entropy of 0
  ASSERT_EQ(addresses.get_entropy(a2), 100);

  // a1 has 1 transaction incoming and 1 outgoin so an entropy of 0 in both
  // cases
  ASSERT_EQ(addresses.get_entropy(a1), 1);
}

TEST_F(Pii, add_block) {}

TEST_F(Pii, get_address_pii) {}

}  // namespace tests
}  // namespace consensus
}  // namespace neuro
