#include <gtest/gtest.h>

#include "config.pb.h"
#include "src/ledger/LedgerMongodb.hpp"
#include "src/messages/Message.hpp"
#include "src/tooling/genblock.hpp"

namespace neuro {
namespace test {

TEST(Blocks, Set_Block1_9){
  neuro::messages::config::Config _config;
  messages::from_json_file("../../bot1.json", &_config);

  auto database = _config.database();
  neuro::ledger::LedgerMongodb _ledger(database);

  for(int i = 1;i < 10;++i)
  {
    neuro::messages::Block _block;
    _block.Clear();
    neuro::tooling::genblock::genblock_from_last_db_block(_block,
            _ledger, 1);


    _ledger.push_block(_block);
  }

  ASSERT_EQ(9 , _ledger.height());
}

}  // namespace test
}  // namespace neuro
