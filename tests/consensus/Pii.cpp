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

  void new_block_3_transactions() {
    messages::TaggedBlock last_block;
    ledger->get_last_block(&last_block);
    auto block = simulator.new_block(0, last_block);
    block.clear_transactions();

    // Create 3 transactions so that addresses 0 and 2 have entropy
    auto transaction = simulator.send_ncc(simulator.keys[0].private_key(),
                                          simulator.addresses[2], 0.5);
    block.add_transactions()->CopyFrom(transaction);
    messages::TaggedTransaction tagged_transaction;
    tagged_transaction.mutable_transaction()->CopyFrom(transaction);
    tagged_transaction.set_is_coinbase(true);
    ledger->add_transaction(tagged_transaction);
    transaction = simulator.send_ncc(simulator.keys[1].private_key(),
                                     simulator.addresses[2], 0.5);
    block.add_transactions()->CopyFrom(transaction);
    tagged_transaction.mutable_transaction()->CopyFrom(transaction);
    tagged_transaction.set_is_coinbase(true);
    ledger->add_transaction(tagged_transaction);
    transaction = simulator.send_ncc(simulator.keys[0].private_key(),
                                     simulator.addresses[1], 0.5);
    block.add_transactions()->CopyFrom(transaction);
    tagged_transaction.mutable_transaction()->CopyFrom(transaction);
    tagged_transaction.set_is_coinbase(true);
    ledger->add_transaction(tagged_transaction);

    messages::sort_transactions(&block);
    messages::set_block_hash(&block);
    ASSERT_TRUE(simulator.consensus->add_block(&block));
    messages::TaggedBlock tagged_block;
    ASSERT_TRUE(ledger->get_last_block(&tagged_block));
    ASSERT_EQ(tagged_block.block().header().height(),
              last_block.block().header().height() + 1);
    ASSERT_EQ(tagged_block.block().transactions_size(), 3);
  }

  void new_block_1_transaction() {
    messages::TaggedBlock last_block;
    ledger->get_last_block(&last_block);
    auto block = simulator.new_block(0, last_block);
    block.clear_transactions();
    auto transaction = simulator.send_ncc(simulator.keys[0].private_key(),
                                          simulator.addresses[1], 0.5);
    block.add_transactions()->CopyFrom(transaction);
    messages::TaggedTransaction tagged_transaction;
    tagged_transaction.mutable_transaction()->CopyFrom(transaction);
    tagged_transaction.set_is_coinbase(true);
    ledger->add_transaction(tagged_transaction);
    messages::sort_transactions(&block);
    messages::set_block_hash(&block);
    ASSERT_TRUE(simulator.consensus->add_block(&block));
    messages::TaggedBlock tagged_block;
    ASSERT_TRUE(ledger->get_last_block(&tagged_block));
    ASSERT_EQ(tagged_block.block().header().height(),
              last_block.block().header().height() + 1);
    ASSERT_EQ(tagged_block.block().transactions_size(), 1);
  }

  void test_get_enthalpy() {
    new_block_1_transaction();
    messages::TaggedBlock last_block;
    ASSERT_TRUE(ledger->get_last_block(&last_block));
    auto &transaction = last_block.block().transactions(0);
    Double enthalpy;
    ASSERT_TRUE(pii.get_enthalpy(transaction, last_block,
                                 simulator.addresses[0], simulator.addresses[1],
                                 &enthalpy));
    ASSERT_GT(enthalpy, 2);
    ASSERT_LT(enthalpy, 50);
  }

  void test_add_block() {
    auto &sender = simulator.addresses[0];
    auto &recipient = simulator.addresses[2];
    messages::TaggedBlock last_block;
    ASSERT_TRUE(ledger->get_last_block(&last_block));
    ASSERT_EQ(last_block.block().header().height(), 0);
    ASSERT_EQ(pii._addresses._addresses.count(recipient), 0);
    ASSERT_EQ(pii._addresses._addresses.count(sender), 0);
    new_block_3_transactions();
    ASSERT_TRUE(ledger->get_last_block(&last_block));
    ASSERT_TRUE(pii.add_block(last_block));
    new_block_3_transactions();
    ASSERT_TRUE(ledger->get_last_block(&last_block));
    ASSERT_TRUE(pii.add_block(last_block));
    ASSERT_EQ(pii._addresses._addresses.count(recipient), 1);
    ASSERT_EQ(pii._addresses._addresses.count(sender), 1);
    ASSERT_EQ(pii._addresses._addresses.at(recipient)._in.size(), 2);
    ASSERT_EQ(pii._addresses._addresses.at(recipient)._out.size(), 0);
    ASSERT_EQ(pii._addresses._addresses.at(sender)._out.size(), 2);
    ASSERT_EQ(pii._addresses._addresses.at(sender)._in.size(), 0);
    ASSERT_EQ(
        pii._addresses._addresses.at(sender)._out.at(recipient).nb_transactions,
        2);
    ASSERT_EQ(pii._addresses._addresses.at(sender)._in.count(recipient), 0);
    ASSERT_EQ(pii._addresses._addresses.at(recipient)._out.count(sender), 0);
    ASSERT_EQ(
        pii._addresses._addresses.at(recipient)._in.at(sender).nb_transactions,
        2);
    for (auto const &entropy : {pii._addresses.get_entropy(sender),
                                pii._addresses.get_entropy(recipient)}) {
      ASSERT_GT(entropy, 2);
      ASSERT_LT(entropy, 50);
    }
    auto addresses_pii = pii.get_addresses_pii();
    ASSERT_EQ(addresses_pii.size(), 3);
    auto &pii0 = addresses_pii[0];
    auto &pii1 = addresses_pii[1];
    auto &pii2 = addresses_pii[2];
    ASSERT_EQ(pii2.address(), simulator.addresses[1]);
    ASSERT_EQ(pii2.score(), 1);
    ASSERT_NE(pii0.address(), pii1.address());
    for (const auto &pii : {pii0, pii1}) {
      ASSERT_TRUE(pii.address() == sender || pii.address() == recipient);
      ASSERT_GT(pii.score(), 2);
      ASSERT_LT(pii.score(), 50);
    }
  }
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

TEST_F(Pii, get_enthalpy) { test_get_enthalpy(); }

TEST_F(Pii, add_block) { test_add_block(); }

}  // namespace tests
}  // namespace consensus
}  // namespace neuro
