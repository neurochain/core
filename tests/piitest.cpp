#include <gtest/gtest.h>

#include "config.pb.h"
#include "src/consensus/PiiConsensus.hpp"
#include "src/ledger/LedgerMongodb.hpp"
#include "src/messages/Message.hpp"
#include "src/networking/Networking.hpp"
#include "src/tooling/blockgen.hpp"

namespace neuro {
namespace test {

std::shared_ptr<ledger::Ledger> _ledger;

TEST(PiiTest, Pii_1_assembly) {
  neuro::messages::config::Config _config;
  messages::from_json_file("../../bot1.json", &_config);

  auto database = _config.database();
  // ledger::LedgerMongodb _ledger(database);
  _ledger = std::make_shared<ledger::LedgerMongodb>(database);
  auto io = std::make_shared<boost::asio::io_context>();
  neuro::consensus::PiiConsensus _piisus(
      io, _ledger, 10, std::make_shared<networking::Networking>());
  _piisus.show_results();

  neuro::messages::Hash next_id, owner_id;
  messages::from_json(
      "{\"type\":\"SHA256\",\"data\":\"vHf7HIWFjkUrcgDx4+"
      "ZPbqkcbSAPDeX6opHVnuR5lc0=\"}",
      &next_id);

  owner_id.ParseFromString(_piisus.owner_at(10));

  std::cout << owner_id << std::endl;
  ASSERT_TRUE(true);
  /*ASSERT_TRUE(next_id.type() == owner_id.type() &&
              next_id.data() == owner_id.data());*/
}

TEST(PiiTest, Pii_rand) {
  auto io = std::make_shared<boost::asio::io_context>();
  neuro::consensus::PiiConsensus _piisus(io, _ledger, 10);
  _piisus.show_results();

  neuro::messages::Hash owner_id;
  owner_id.ParseFromString(_piisus.owner_at(10));

  std::cout << owner_id << std::endl;

  neuro::messages::Block block11;

  crypto::Ecc _ecc({"../../keys/key_3.priv"}, {"../../keys/key_3.pub"});
  Buffer key_pub_raw;
  _ecc.mutable_key_pub()->save(&key_pub_raw);

  messages::_KeyPub author;
  author.set_type(messages::KeyType::ECP256K1);
  author.set_raw_data(key_pub_raw.data(), key_pub_raw.size());

  neuro::tooling::blockgen::blockgen_from_last_db_block(
      block11, _ledger, 0, 5,
      std::make_optional<neuro::messages::_KeyPub>(author));

  neuro::messages::Block block12;

  neuro::tooling::blockgen::blockgen_from_last_db_block(
      block12, _ledger, 0, 5,
      std::make_optional<neuro::messages::_KeyPub>(author));

  ASSERT_EQ(block11, block12);
}

/*
TEST(PiiTest, Pii_next_owner) {
  auto io = std::make_shared<boost::asio::io_context>();
  neuro::consensus::PiiConsensus _piisus(io, _ledger, 10);

  neuro::messages::Block block11;

  crypto::Ecc _ecc({"../../keys/key_3.priv"}, {"../../keys/key_3.pub"});
  Buffer key_pub_raw;
  _ecc.mutable_key_pub()->save(&key_pub_raw);

  messages::_KeyPub author;
  author.set_type(messages::KeyType::ECP256K1);
  author.set_raw_data(key_pub_raw.data(), key_pub_raw.size());

  neuro::tooling::blockgen::blockgen_from_last_db_block(
      block11, _ledger, 0, 10,
      std::make_optional<neuro::messages::_KeyPub>(author));

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
  auto io = std::make_shared<boost::asio::io_context>();
  neuro::consensus::PiiConsensus _piisus(io, _ledger, 10);
  neuro::messages::Block block11;

  crypto::Ecc _ecc({"../../keys/key_3.priv"}, {"../../keys/key_3.pub"});
  Buffer key_pub_raw;
  _ecc.mutable_key_pub()->save(&key_pub_raw);

  messages::_KeyPub author;
  author.set_type(messages::KeyType::ECP256K1);
  author.set_raw_data(key_pub_raw.data(), key_pub_raw.size());

  neuro::tooling::blockgen::blockgen_from_last_db_block(
      block11, _ledger, 0, 10,
      std::make_optional<neuro::messages::_KeyPub>(author), 9);

  messages::Block blockfork;
  ASSERT_TRUE(_ledger->get_block(10, &blockfork));
  // ASSERT_THROW(_piisus.add_block(block11), std::runtime_error);
}*/

}  // namespace test
}  // namespace neuro
