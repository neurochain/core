#include <gtest/gtest.h>
#include <boost/filesystem/path.hpp>

#include "config.pb.h"
#include "messages.pb.h"
#include "messages/Message.hpp"
#include "messages/config/Config.hpp"

namespace neuro {
namespace test {

TEST(Conf, load) {
  bool did_throw(false);
  Path config_path("./bot2.json");
  try {
    auto configuration = messages::config::Config{config_path};
  } catch (...) {
    did_throw = true;
  }
  ASSERT_FALSE(did_throw);

  auto conf = messages::config::Config{config_path};

  boost::filesystem::path pub_path(conf.networking().key_pub_path());
  boost::filesystem::path priv_path(conf.networking().key_priv_path());
  ASSERT_EQ(pub_path.filename(), "key2.pub");
  ASSERT_EQ(priv_path.filename(), "key2.priv");

  ASSERT_TRUE(conf.has_networking());
  const auto& networking = conf.networking();
  ASSERT_EQ(networking.max_connections(), 3);

  ASSERT_TRUE(networking.has_tcp());
  const auto& tcp = networking.tcp();
  ASSERT_TRUE(tcp.has_port());
  ASSERT_EQ(tcp.port(), 1339);

  ASSERT_EQ(tcp.peers_size(), 2);
  const std::string endpoint("localhost");

  for (const auto& peer : tcp.peers()) {
    ASSERT_TRUE(peer.has_endpoint());
    ASSERT_EQ(peer.endpoint(), endpoint);
    ASSERT_TRUE(peer.has_port());
    ASSERT_TRUE(peer.has_key_pub());
    const messages::_KeyPub& kp(peer.key_pub());
    ASSERT_TRUE(kp.has_type());
    ASSERT_EQ(kp.type(), messages::ECP256K1);
    ASSERT_TRUE(kp.has_raw_data());
  }

  const messages::Peer& peer0(tcp.peers(0));
  ASSERT_EQ(peer0.port(), 1337);

  const messages::Peer& peer1(tcp.peers(1));
  ASSERT_EQ(peer1.port(), 1338);
}

}  // namespace test
}  // namespace neuro
