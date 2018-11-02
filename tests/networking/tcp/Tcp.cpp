#include "networking/tcp/Tcp.hpp"
#include <gtest/gtest.h>
#include "crypto/Ecc.hpp"
#include "messages/Message.hpp"
#include "networking/Networking.hpp"
#include "networking/TransportLayer.hpp"

namespace neuro {

namespace test {

class Tcp {
 public:
  Tcp() = default;

  void test_connection() {
    auto queue = std::make_shared<messages::Queue>();
    auto keys1 = std::make_shared<crypto::Ecc>(),
         keys2 = std::make_shared<crypto::Ecc>();
    networking::Tcp tcp1(queue, keys1);
    networking::Tcp tcp2(queue, keys2);
    Port port{31212};  // Maybe change this to the port to be used by the bot
    tcp1.accept(port);
    auto peer = std::make_shared<messages::Peer>();
    peer->set_endpoint("127.0.0.1");
    peer->set_port(port);
    tcp2.connect(peer);
    tcp1._io_service.run_one();
    tcp2._io_service.run_one();
    ASSERT_EQ(tcp1._connections.size(), 1);
    ASSERT_EQ(tcp2._connections.size(), 1);
  }
};

TEST(Tcp, ConnectionTest) {
  Tcp tcp_test;
  tcp_test.test_connection();
}

}  // namespace test
}  // namespace neuro
