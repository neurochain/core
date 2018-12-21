#include "networking/tcp/Tcp.hpp"
#include <gtest/gtest.h>
#include "crypto/Ecc.hpp"
#include "messages/Message.hpp"
#include "networking/Networking.hpp"
#include "networking/TransportLayer.hpp"

namespace neuro {
namespace networking {
namespace test {

class Tcp {
 public:
  Tcp() = default;

  void test_listen() {
    auto queue = std::make_shared<messages::Queue>();
    ASSERT_NE(queue, nullptr);
    auto keys1 = std::make_shared<crypto::Ecc>();
    Port port{31212};  // Maybe change this to the port to be used by the bot
    auto peer_pool = std::make_shared<PeerPool>("peers1.json");
    networking::Tcp tcp1(port, queue, keys1, peer_pool, 3);
    std::this_thread::sleep_for(std::chrono::seconds(2));
    ASSERT_EQ(tcp1._connection_pool.size(), 0);
    tcp1.stop();
  }

  void test_connection() {
    auto queue = std::make_shared<messages::Queue>();
    ASSERT_NE(queue, nullptr);
    auto keys1 = std::make_shared<crypto::Ecc>(),
         keys2 = std::make_shared<crypto::Ecc>();
    Port port{31212};  // Maybe change this to the port to be used by the bot
    auto peer_pool1 = std::make_shared<PeerPool>("peers1.json");
    auto peer_pool2 = std::make_shared<PeerPool>("peers2.json");
    networking::Tcp tcp1(port, queue, keys1, peer_pool1, 3);
    networking::Tcp tcp2(port + 1, queue, keys2, peer_pool2, 3);
    messages::Peer peer;
    peer.set_endpoint("127.0.0.1");
    peer.set_port(port);
    auto key_pub = peer.mutable_key_pub();
    keys1->public_key().save(key_pub);
    tcp2.connect(peer);
    std::this_thread::sleep_for(std::chrono::seconds(2));
    ASSERT_EQ(tcp1._connection_pool.size(), 1);
    ASSERT_EQ(tcp2._connection_pool.size(), 1);
    tcp1.stop();
    tcp2.stop();
  }
};

TEST(Tcp, listen) {
  networking::test::Tcp tcp_test;
  tcp_test.test_listen();
}

TEST(Tcp, connection) {
  networking::test::Tcp tcp_test;
  tcp_test.test_connection();
}

}  // namespace test
}  // namespace networking
}  // namespace neuro
