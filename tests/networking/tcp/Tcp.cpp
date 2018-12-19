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
    networking::Tcp tcp1(port, queue, keys1, 3);
    std::this_thread::sleep_for(std::chrono::seconds(1));
    ASSERT_EQ(tcp1._connection_pool.size(), 1);
  }

  void test_connection() {
    auto queue = std::make_shared<messages::Queue>();
    ASSERT_NE(queue, nullptr);
    auto keys1 = std::make_shared<crypto::Ecc>(),
         keys2 = std::make_shared<crypto::Ecc>();
    Port port{31212};  // Maybe change this to the port to be used by the bot
    networking::Tcp tcp1(port, queue, keys1, 3);
    networking::Tcp tcp2(port + 1, queue, keys2, 3);
    messages::Peer peer;
    peer.set_endpoint("127.0.0.1");
    peer.set_port(port);
    tcp2.connect(peer);
    std::this_thread::sleep_for(std::chrono::seconds(500));
    ASSERT_EQ(tcp1._connection_pool.size(), 1);
    ASSERT_EQ(tcp2._connection_pool.size(), 1);
  }
};

TEST(Tcp, connection_test) {
  networking::test::Tcp tcp_test;
  tcp_test.test_connection();
}

TEST(Tcp, listen) {
  networking::test::Tcp tcp_test;
  tcp_test.test_listen();
}

}  // namespace test
}  // namespace networking
}  // namespace neuro
