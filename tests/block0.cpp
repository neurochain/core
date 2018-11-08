#include <gtest/gtest.h>

#include "config.pb.h"
#include "messages/config/Config.hpp"
#include "src/ledger/LedgerMongodb.hpp"
#include "src/messages/Message.hpp"
#include "src/tooling/genblock.hpp"

namespace neuro {
namespace test {

/*
TEST(Blocks, Set_Block0) {
  neuro::messages::config::Config _config;
  messages::from_json_file("../../bot1.json", &_config);

  auto database = _config.database();
  ledger::LedgerMongodb _ledger(database);

  messages::Block _block;
  bool res = _ledger.get_block(0, &_block);
  ASSERT_EQ(true, res);
}*/

TEST(Blocks, Set_Find) {
  neuro::messages::config::Config _config;
  messages::from_json_file("../../bot1.json", &_config);

  auto database = _config.database();
  ledger::LedgerMongodb _ledger(database);

  messages::Block _block;
  bool res = _ledger.get_block(0, &_block);

  messages::Block block;

  res = _ledger.get_block_by_previd(_block.header().id(), &block);

  ASSERT_EQ(true, res);
}
}  // namespace test
}  // namespace neuro
