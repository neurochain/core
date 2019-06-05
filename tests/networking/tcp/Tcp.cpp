#include "networking/tcp/Tcp.hpp"
#include "common.pb.h"
#include "crypto/Ecc.hpp"
#include "messages/Message.hpp"
#include "messages/Peers.hpp"
#include "networking/Networking.hpp"
#include "networking/TransportLayer.hpp"
#include <gtest/gtest.h>
#include <src/messages/config/Config.hpp>

namespace neuro {
namespace networking {
namespace test {

class Tcp {
 public:
  Tcp() = default;

  void test_connection() {
    auto queue = messages::Queue{};
    crypto::Ecc keys1;
    crypto::Ecc keys2;

    auto conf1 = messages::config::Config{Path("bot1.json")};
    auto conf2 = messages::config::Config{Path("bot2.json")};
    messages::Peer peer1(conf1.networking());
    peer1.set_endpoint("localhost");
    peer1.set_port(conf1.networking().tcp().port());
    messages::Peer peer2(conf2.networking());
    peer2.set_endpoint(conf2.networking().tcp().endpoint());
    peer2.set_port(conf2.networking().tcp().port());
    messages::Peers peers(peer1.key_pub(), conf1.networking());
    networking::Tcp tcp1(&queue, &peers, &keys1, conf1.networking());
    networking::Tcp tcp2(&queue, &peers, &keys2, conf2.networking());
    std::this_thread::sleep_for(std::chrono::milliseconds(5000));
    tcp2.connect(&peer1);
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
