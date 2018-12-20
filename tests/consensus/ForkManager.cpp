#include "consensus/ForkManager.hpp"
#include <gtest/gtest.h>
#include <chrono>
#include <sstream>
#include <thread>
#include "Bot.hpp"
#include "common/logger.hpp"
#include "common/types.hpp"
#include "consensus/TransactionPool.hpp"
#include "ledger/LedgerMongodb.hpp"
#include "messages/config/Config.hpp"
#include "messages/config/Database.hpp"
#include "tooling/genblock.hpp"

namespace neuro {
namespace consensus {
namespace tests {

class ForkManager : public ::testing::Test {
 public:
  const std::string config_str =
      "{"
      "  \"url\": \"mongodb://localhost:27017\","
      "  \"dbName\": \"neuro_test\","
      "  \"block0Format\": \"PROTO\","
      "  \"block0Path\": \"./data.0.testnet\""
      "}";

 protected:
  messages::config::Database _config;
  std::shared_ptr<::neuro::ledger::LedgerMongodb> ledger;

  ForkManager() : _config(config_str) {
    tooling::genblock::create_empty_db(_config);
    ledger = std::make_shared<::neuro::ledger::LedgerMongodb>(_config);
  }

  std::string db_name() const { return _config.db_name(); }

  std::string db_url() const { return _config.url(); }

  messages::config::Database config() { return _config; }
};

TEST_F(ForkManager, merge_fork_blocks) {
  tooling::genblock::create_first_blocks(5, ledger);
  tooling::genblock::create_fork_blocks(5, ledger);
  ASSERT_EQ(ledger->height(), 4);
  ASSERT_EQ(ledger->total_nb_blocks(), 5);
  auto shared_ledger = std::make_shared<ledger::LedgerMongodb>(_config);
  auto transaction_pool = TransactionPool(shared_ledger);
  auto fork_manager =
      neuro::consensus::ForkManager(shared_ledger, transaction_pool);
  fork_manager.merge_fork_blocks();
  ASSERT_EQ(ledger->total_nb_blocks(), 10);
  ASSERT_EQ(ledger->height(), 9);
}

TEST_F(ForkManager, fork_status) {
  tooling::genblock::create_first_blocks(5, ledger);
  tooling::genblock::create_fork_blocks(5, ledger);
  messages::Block last_block, previous_block;
  ledger->get_block(ledger->height(), &last_block);
  ledger->get_block(ledger->height() - 1, &previous_block);
  auto shared_ledger = std::make_shared<ledger::LedgerMongodb>(_config);
  auto transaction_pool = TransactionPool(shared_ledger);
  auto fork_manager =
      neuro::consensus::ForkManager(shared_ledger, transaction_pool);
  auto status = fork_manager.fork_status(
      last_block.header(), previous_block.header(), last_block.header());
  ASSERT_EQ(status, neuro::consensus::ForkManager::ForkStatus::Dual_Block);
  // TODO make tests that make sense fork_status makes no sense to me
}

TEST_F(ForkManager, fork_result_no_fork) {
  tooling::genblock::create_first_blocks(5, ledger);
  tooling::genblock::create_fork_blocks(5, ledger);
  tooling::genblock::create_more_blocks(10, ledger);
  ASSERT_EQ(ledger->total_nb_blocks(), 15);

  auto shared_ledger = std::make_shared<ledger::LedgerMongodb>(_config);
  auto transaction_pool = TransactionPool(shared_ledger);
  auto fork_manager =
      neuro::consensus::ForkManager(shared_ledger, transaction_pool);
  ASSERT_EQ(fork_manager.fork_results(), false);
  ASSERT_EQ(ledger->total_nb_blocks(), 15);
}

TEST_F(ForkManager, fork_result_fork) {
  tooling::genblock::create_first_blocks(5, ledger);
  tooling::genblock::create_fork_blocks(10, ledger);
  tooling::genblock::create_more_blocks(5, ledger);
  ASSERT_EQ(ledger->total_nb_blocks(), 10);

  auto shared_ledger = std::make_shared<ledger::LedgerMongodb>(_config);
  auto transaction_pool = TransactionPool(shared_ledger);
  auto fork_manager =
      neuro::consensus::ForkManager(shared_ledger, transaction_pool);
  ASSERT_EQ(fork_manager.fork_results(), true);
  ASSERT_EQ(ledger->height(), 14);
  ASSERT_EQ(ledger->total_nb_blocks(), 15);
}

}  // namespace tests
}  // namespace consensus
}  // namespace neuro
