#include "networking/tcp/Tcp.hpp"
#include <gtest/gtest.h>
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
    auto keys1 = std::make_shared<crypto::Ecc>(),
         keys2 = std::make_shared<crypto::Ecc>();

    messages::Peers peers;
    Port port{31212};  // Maybe change this to the port to be used by the bot
    auto peer = std::make_shared<messages::Peer>();
    peer->set_endpoint("127.0.0.1");
    peer->set_port(port);

    networking::Tcp tcp1(port, 1, &queue, &peers, keys1);
    networking::Tcp tcp2(port + 1, 2, &queue, &peers, keys2);
    tcp2.connect(peer);
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
