#include <gtest/gtest.h>

#include "config.pb.h"
#include "src/ledger/LedgerMongodb.hpp"
#include "src/messages/Message.hpp"

namespace neuro {
namespace test {

TEST(Config, test_Config) {
  neuro::messages::config::Config _config;
  messages::from_json_file("../../bot1.json", &_config);
  ASSERT_EQ("neuro", _config.database().db_name());
}

TEST(Database, Remove_database) {
  neuro::messages::config::Config _config;
  messages::from_json_file("../../bot1.json", &_config);

  ledger::LedgerMongodb _ledger(_config.database().url(),
                                _config.database().db_name());
  _ledger.remove_all();

  messages::Block _block;
  bool res = _ledger.get_block(0, &_block);
  ASSERT_EQ(false, res);
}

}  // namespace test
}  // namespace neuro
