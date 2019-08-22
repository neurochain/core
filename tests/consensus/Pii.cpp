#include <gtest/gtest.h>

#include "consensus/Pii.hpp"
#include "ledger/LedgerMongodb.hpp"
#include "messages/NCCAmount.hpp"
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
      : simulator(tooling::Simulator::Simulator::StaticSimulator(
            db_url, db_name, nb_keys, ncc_block0)),
        ledger(simulator.ledger),
        pii(ledger, simulator.consensus->config()) {}

  void new_block_3_transactions() {
    messages::TaggedBlock last_block;
    ledger->get_last_block(&last_block);

    // Create 3 transactions so that key_pubs 0 and 2 have entropy
    auto transaction = ledger->send_ncc(simulator.keys[0].key_priv(),
                                        simulator.key_pubs[2], 0.5);
    simulator.consensus->add_transaction(transaction);
    transaction = ledger->send_ncc(simulator.keys[1].key_priv(),
                                   simulator.key_pubs[2], 0.5);
    simulator.consensus->add_transaction(transaction);
    transaction = ledger->send_ncc(simulator.keys[0].key_priv(),
                                   simulator.key_pubs[1], 0.5);
    simulator.consensus->add_transaction(transaction);

    auto block = simulator.new_block(last_block);
    ASSERT_TRUE(simulator.consensus->add_block(block));
    messages::TaggedBlock tagged_block;
    ASSERT_TRUE(ledger->get_last_block(&tagged_block));
    ASSERT_EQ(tagged_block.block().header().height(),
              last_block.block().header().height() + 1);
    ASSERT_EQ(tagged_block.block().transactions_size(), 3);
  }

  void new_block_1_transaction() {
    messages::TaggedBlock last_block;
    ledger->get_last_block(&last_block);
    auto transaction = ledger->send_ncc(simulator.keys[0].key_priv(),
                                        simulator.key_pubs[1], 0.5);
    simulator.consensus->add_transaction(transaction);
    auto block = simulator.new_block(last_block);
    ASSERT_TRUE(simulator.consensus->add_block(block));
    messages::TaggedBlock tagged_block;
    ASSERT_TRUE(ledger->get_last_block(&tagged_block));
    ASSERT_EQ(tagged_block.block().header().height(),
              last_block.block().header().height() + 1);
    ASSERT_EQ(tagged_block.block().transactions_size(), 1);
  }

  void test_add_block() {
    auto &sender = simulator.key_pubs[0];
    auto &recipient = simulator.key_pubs[2];
    messages::TaggedBlock last_block;
    ASSERT_TRUE(ledger->get_last_block(&last_block));
    ASSERT_EQ(last_block.block().header().height(), 0);
    ASSERT_EQ(pii._key_pubs._key_pubs.count(recipient), 0);
    ASSERT_EQ(pii._key_pubs._key_pubs.count(sender), 0);
    new_block_3_transactions();
    ASSERT_TRUE(ledger->get_last_block(&last_block));
    ASSERT_TRUE(pii.add_block(last_block));
    ASSERT_EQ(
        pii._key_pubs._key_pubs.at(sender)._out.at(recipient).nb_transactions,
        1);

    new_block_3_transactions();

    ASSERT_TRUE(ledger->get_last_block(&last_block));
    ASSERT_TRUE(pii.add_block(last_block));
    ASSERT_EQ(pii._key_pubs._key_pubs.count(recipient), 1);
    ASSERT_EQ(pii._key_pubs._key_pubs.count(sender), 1);
    ASSERT_EQ(pii._key_pubs._key_pubs.at(recipient)._in.size(), 2);
    ASSERT_EQ(pii._key_pubs._key_pubs.at(recipient)._out.size(), 0);
    ASSERT_EQ(pii._key_pubs._key_pubs.at(sender)._out.size(), 2);
    ASSERT_EQ(pii._key_pubs._key_pubs.at(sender)._in.size(), 0);
    ASSERT_EQ(
        pii._key_pubs._key_pubs.at(sender)._out.at(recipient).nb_transactions,
        2);
    ASSERT_EQ(pii._key_pubs._key_pubs.at(sender)._in.count(recipient), 0);
    ASSERT_EQ(pii._key_pubs._key_pubs.at(recipient)._out.count(sender), 0);
    ASSERT_EQ(
        pii._key_pubs._key_pubs.at(recipient)._in.at(sender).nb_transactions,
        2);
    for (auto const &entropy : {pii._key_pubs.get_entropy(sender),
                                pii._key_pubs.get_entropy(recipient)}) {
      ASSERT_GT(entropy, 2);
      ASSERT_LT(entropy, 50);
    }
    auto assembly_height = 0;
    auto key_pubs_pii =
        pii.get_key_pubs_pii(assembly_height, last_block.branch_path());
    ASSERT_EQ(key_pubs_pii.size(), 3);
    auto &pii0 = key_pubs_pii[0];
    auto &pii1 = key_pubs_pii[1];
    auto &pii2 = key_pubs_pii[2];
    ASSERT_EQ(pii2.key_pub(), simulator.key_pubs[1]);
    ASSERT_EQ(pii2.score(), "1");
    ASSERT_NE(pii0.key_pub(), pii1.key_pub());
    for (const auto &pii : {pii0, pii1}) {
      ASSERT_TRUE(pii.key_pub() == sender || pii.key_pub() == recipient);
      ASSERT_GT(Double(pii.score()), 2);
      ASSERT_LT(Double(pii.score()), 50);
    }
  }
};

TEST(KeyPubs, get_pii) {
  KeyPubs key_pubs;
  crypto::Ecc ecc0, ecc1, ecc2;
  auto a0 = ecc0.key_pub();
  auto a1 = ecc1.key_pub();
  auto a2 = ecc2.key_pub();
  key_pubs.add_enthalpy(a0, a1, 100);

  // Entropy is 0 when there is only 1 transaction and floored to 1
  ASSERT_EQ(key_pubs.get_entropy(a0), 1);

  // Entropy is 100 because -0.5 * log2(0.5) == 1
  key_pubs.add_enthalpy(a0, a2, 100);

  ASSERT_EQ(key_pubs.get_entropy(a0), 100);
  ASSERT_EQ(key_pubs.get_entropy(a1), 1);
  ASSERT_EQ(key_pubs.get_entropy(a2), 1);
  key_pubs.add_enthalpy(a1, a2, 100);

  // a2 should have an outgoing entropy of 0 and incoming entropy of 0
  ASSERT_EQ(key_pubs.get_entropy(a2), 100);

  // a1 has 1 transaction incoming and 1 outgoin so an entropy of 0 in both
  // cases
  ASSERT_EQ(key_pubs.get_entropy(a1), 1);
}

TEST_F(Pii, add_block) { test_add_block(); }

}  // namespace tests
}  // namespace consensus
}  // namespace neuro
