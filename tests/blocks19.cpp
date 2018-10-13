#include <gtest/gtest.h>

#include "config.pb.h"
#include "src/ledger/LedgerMongodb.hpp"
#include "src/messages/Message.hpp"
#include "src/tooling/genblock.hpp"

namespace neuro {
namespace test {

TEST(Blocks, Set_Block1_9) {
  messages::config::Config _config;
  messages::from_json_file("../../bot1.json", &_config);

  auto database = _config.database();
  auto ledger = std::make_shared<ledger::LedgerMongodb>(database);

  for (int i = 1; i < 10; ++i) {
    messages::Block block;
    block.Clear();
    tooling::genblock::genblock_from_last_db_block(block, ledger, 1, i );

    ledger->push_block(block);
  }

  ASSERT_EQ(9, ledger->height());
}

}  // namespace test
}  // namespace neuro
