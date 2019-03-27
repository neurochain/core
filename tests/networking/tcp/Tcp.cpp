#include "networking/tcp/Tcp.hpp"
#include <gtest/gtest.h>
#include "common.pb.h"
#include "crypto/Ecc.hpp"
#include "messages/Message.hpp"
#include "messages/Peers.hpp"
#include "networking/Networking.hpp"
#include "networking/TransportLayer.hpp"

namespace neuro {
namespace networking {
namespace test {

class Tcp {
 public:
  Tcp() = default;

  void test_connection() {
    auto queue = messages::Queue{};
    auto keys0 = std::make_shared<crypto::Ecc>();
    auto keys1 = std::make_shared<crypto::Ecc>();
    auto keys2 = std::make_shared<crypto::Ecc>();
    auto key_writer = std::make_shared<crypto::Ecc>();

    messages::_KeyPub own_key;
    key_writer->key_pub().save(&own_key);
    messages::Peers peers(own_key);
    Port port{31212};  // Maybe change this to the port to be used by the bot
    messages::Peer peer;
    peer.set_endpoint("127.0.0.1");
    peer.set_port(port);
    keys2->key_pub().save(peer.mutable_key_pub());
    networking::Tcp tcp1(port, &queue, &peers, keys1.get());
    networking::Tcp tcp2(port + 1, &queue, &peers, keys2.get());
    std::this_thread::sleep_for(std::chrono::milliseconds(5000));
    tcp2.connect(&peer);
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    ASSERT_EQ(tcp1._connections.size(), 1);
    ASSERT_EQ(tcp2._connections.size(), 1);
  }
};

TEST(Tcp, ConnectionTest) {
  networking::test::Tcp tcp_test;
  tcp_test.test_connection();
}

}  // namespace test
}  // namespace networking
}  // namespace neuro
