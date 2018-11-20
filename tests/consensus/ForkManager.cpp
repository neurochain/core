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
      "  \"url\": \"mongodb://mongo:27017\","
      "  \"dbName\": \"neuro_test\","
      "  \"block0Format\": \"PROTO\","
      "  \"block0Path\": \"./data.0.testnet\""
      "}";

 protected:
  messages::config::Database _config;
  std::shared_ptr<::neuro::ledger::LedgerMongodb> ledger;

  void create_empty_db(const messages::config::Database &config) {
    // instance started in static in LedgerMongodb
    mongocxx::uri uri(config.url());
    mongocxx::client client(uri);
    mongocxx::database db(client[config.db_name()]);
    db.drop();
  }

  ForkManager() : _config(config_str) {
    create_empty_db(_config);
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

}  // namespace tests
}  // namespace consensus
}  // namespace neuro
