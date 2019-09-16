#include "tooling/FullSimulator.hpp"
#include <gtest/gtest.h>

namespace neuro {
namespace tooling {
namespace tests {

class FullSimulator : public ::testing::Test {
 public:
  const std::string db_url = "mongodb://mongo:27017";
  const std::string db_name = "test_simulator";
  const messages::NCCAmount ncc_block0 = messages::NCCAmount(1000000);
  int nb_bots = 4;

 protected:
  void check_bots() {
    tooling::FullSimulator simulator(db_url, db_name, nb_bots, ncc_block0);
    ASSERT_EQ(simulator.bots.size(), nb_bots);
    std::this_thread::sleep_for(30s);
    for (const auto &bot : simulator.bots) {
      ASSERT_GT(bot->connected_peers().size(), 2);
      auto height = bot->_ledger->height();
      ASSERT_GT(height, 15);
      ASSERT_LT(height, 30);
      auto total_nb_blocks = bot->_ledger->total_nb_blocks();
      ASSERT_GT(total_nb_blocks, 15);
      ASSERT_LT(total_nb_blocks, 30);
      auto total_nb_transactions = bot->_ledger->total_nb_transactions();
      ASSERT_GT(total_nb_transactions, 30);
    }
  }

  void check_send_transaction() {
    consensus::Config consensus_config = default_consensus_config;
    consensus_config.block_period = 3600;
    double random_transaction = 0;
    nb_bots = 2;
    tooling::FullSimulator simulator(db_url, db_name, nb_bots, ncc_block0,
                                     consensus_config, random_transaction);
    ASSERT_EQ(simulator.bots.size(), nb_bots);
    std::this_thread::sleep_for(10s);
    for (const auto &bot : simulator.bots) {
      ASSERT_EQ(bot->connected_peers().size(), nb_bots - 1);
      auto height = bot->_ledger->height();
      ASSERT_EQ(height, 0);
      auto transactions = bot->ledger()->get_transaction_pool();
      ASSERT_EQ(transactions.size(), 0);
    }
    simulator.bots[0]->send_random_transaction();
    std::this_thread::sleep_for(1s);
    for (const auto &bot : simulator.bots) {
      ASSERT_EQ(bot->connected_peers().size(), nb_bots - 1);
      auto height = bot->_ledger->height();
      ASSERT_EQ(height, 0);
      auto transactions = bot->ledger()->get_transaction_pool();
      ASSERT_EQ(transactions.size(), 1);
    }
    for (int i = 0; i < 10; i++) {
      simulator.bots[1]->send_random_transaction();
    }
    std::this_thread::sleep_for(1s);
    for (const auto &bot : simulator.bots) {
      ASSERT_EQ(bot->connected_peers().size(), nb_bots - 1);
      auto height = bot->_ledger->height();
      ASSERT_EQ(height, 0);
      auto transactions = bot->ledger()->get_transaction_pool();
      ASSERT_EQ(transactions.size(), 11);
    }
  }
};

TEST_F(FullSimulator, check_bots) { check_bots(); }

TEST_F(FullSimulator, check_send_transaction) { check_send_transaction(); }

}  // namespace tests
}  // namespace tooling
}  // namespace neuro
