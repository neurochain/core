#include <gtest/gtest.h>

#include "config.pb.h"
#include "src/consensus/PiiConsensus.hpp"
#include "src/ledger/LedgerMongodb.hpp"
#include "src/messages/Message.hpp"
#include "src/tooling/genblock.hpp"

namespace neuro {
namespace test {

std::shared_ptr<ledger::Ledger> _ledger;

TEST(PiiTest, Pii_1_assembly) {
  neuro::messages::config::Config _config;
  messages::from_json_file("../../bot1.json", &_config);

  auto database = _config.database();
  // ledger::LedgerMongodb _ledger(database);
  _ledger = std::make_shared<ledger::LedgerMongodb>(database);

  neuro::consensus::PiiConsensus _piisus(_ledger, 10);

  neuro::messages::Hash next_id, owner_id;
  messages::from_json(
      "{\"type\":\"SHA256\",\"data\":\"vHf7HIWFjkUrcgDx4+"
      "ZPbqkcbSAPDeX6opHVnuR5lc0=\"}",
      &next_id);

  owner_id.ParseFromString(_piisus.get_next_owner());

  ASSERT_TRUE(next_id.type() == owner_id.type() &&
              next_id.data() == owner_id.data());
}

TEST(PiiTest, Pii_next_owner) {
  neuro::consensus::PiiConsensus _piisus(_ledger, 10);

  neuro::messages::Block block11;

  crypto::Ecc _ecc({"../../keys/key_3.priv"}, {"../../keys/key_3.pub"});
  Buffer key_pub_raw;
  _ecc.mutable_public_key()->save(&key_pub_raw);

  messages::KeyPub author;
  author.set_type(messages::KeyType::ECP256K1);
  author.set_raw_data(key_pub_raw.data(), key_pub_raw.size());

  neuro::tooling::genblock::genblock_from_last_db_block(
      block11, _ledger, 0, 10,
      std::make_optional<neuro::messages::KeyPub>(author));

  _piisus.add_block(block11);

  neuro::messages::Block lastblock;
  bool res = _ledger->get_block(10, &lastblock);
  ASSERT_TRUE(res);

  neuro::messages::Hash next_addr;
  messages::from_json(
      "{\"type\":\"SHA256\",\"data\":\"vHf7HIWFjkUrcgDx4+"
      "ZPbqkcbSAPDeX6opHVnuR5lc0=\"}",
      &next_addr);

  auto addr = neuro::Buffer{lastblock.header().author().raw_data()};
  neuro::messages::Hasher owner_addr(addr);

  ASSERT_TRUE(next_addr.type() == owner_addr.type() &&
              next_addr.data() == owner_addr.data());
}

TEST(PiiTest, Pii_first_fork) {
  neuro::consensus::PiiConsensus _piisus(_ledger, 10);
  neuro::messages::Block block11;

  crypto::Ecc _ecc({"../../keys/key_3.priv"}, {"../../keys/key_3.pub"});
  Buffer key_pub_raw;
  _ecc.mutable_public_key()->save(&key_pub_raw);

  messages::KeyPub author;
  author.set_type(messages::KeyType::ECP256K1);
  author.set_raw_data(key_pub_raw.data(), key_pub_raw.size());

  neuro::tooling::genblock::genblock_from_last_db_block(
      block11, _ledger, 0, 10,
      std::make_optional<neuro::messages::KeyPub>(author), 9);

  messages::Block blockfork;
  ASSERT_TRUE(_ledger->get_block(10, &blockfork));
  // ASSERT_THROW(_piisus.add_block(block11), std::runtime_error);
}

}  // namespace test
}  // namespace neuro
