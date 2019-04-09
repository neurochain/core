#include <gtest/gtest.h>
#include <chrono>
#include <sstream>
#include <thread>
#include "Bot.hpp"
#include "common/logger.hpp"
#include "common/types.hpp"
#include "crypto/Sign.hpp"
#include "ledger/LedgerMongodb.hpp"
#include "messages/Subscriber.hpp"
#include "messages/config/Config.hpp"
#include "tooling/blockgen.hpp"

namespace neuro {
namespace tests {

using namespace std::chrono_literals;

template <class C>
std::vector<typename C::value_type> vectorize(const C &container) {
  std::vector<typename C::value_type> ans(container.begin(), container.end());
  return ans;
}


template<typename A, typename B>
bool TRAXERT_EQ(const A &a, const B &b, int retry = 10) {
  while (retry --> 0) {
    if (a == b) {
      return true;
    }

    LOG_TRACE << "needed to retry";
    std::this_thread::sleep_for(1s);
  }
  return false;
}
  
class BotTest {
 private:
  std::unique_ptr<neuro::Bot> _bot;

 public:
  BotTest(const std::string configPath,
          const int max_incoming_connections = 3) {
    Path configAsPath(configPath);
    messages::config::Config config(configAsPath);
    _bot = std::make_unique<Bot>(config);
    _bot->_max_incoming_connections = max_incoming_connections;
    _bot->_max_connections = max_incoming_connections;
  }

  void set_max_incoming_connections(int max_incoming_connections) {
    _bot->_max_incoming_connections = max_incoming_connections;
    _bot->_max_connections = max_incoming_connections;
  }

  void keep_max_connections() { _bot->keep_max_connections(); }

  auto me() { return _bot->_me; }

  auto &operator-> () { return _bot; }

  void update_unreachable() { _bot->_peers.update_unreachable(); }

  messages::Peers &peers() { return _bot->_peers; }
  auto &networking() { return _bot->_networking; }

  bool check_peers_ports(std::vector<int> ports) {
    std::sort(ports.begin(), ports.end());
    std::vector<int> peers_ports;
    for (const auto &peer : _bot->connected_peers()) {
      peers_ports.push_back(peer->port());
    }
    std::sort(peers_ports.begin(), peers_ports.end());
    EXPECT_EQ(peers_ports, ports);
    return peers_ports == ports;
  }

  bool check_peer_status_by_port(const Port port,
				 const messages::Peer::Status status) {
    const auto remote = _bot->peers().peer_by_port(port);
    EXPECT_TRUE(remote);
    if(!remote) {
      return false;
    }
    EXPECT_EQ((*remote)->status(), status);
    if((*remote)->status() != status) {
      return false;
    }

    return true;
  }

  bool check_peers_ports_without_fail(std::vector<int> ports) {
    std::sort(ports.begin(), ports.end());
    std::vector<int> peers_ports;
    for (const auto &peer : _bot->connected_peers()) {
      peers_ports.push_back(peer->port());
    }
    std::sort(peers_ports.begin(), peers_ports.end());
    return peers_ports == ports;
  }
  
  int nb_blocks() { return _bot->_ledger->total_nb_blocks(); }
  void add_block() {
    messages::TaggedBlock new_block;
    neuro::tooling::blockgen::blockgen_from_last_db_block(
        new_block.mutable_block(), _bot->_ledger, 1, 0);
    _bot->_ledger->insert_block(new_block);
  }
};

template<class F, class Duration>
bool sleep_until(F f, Duration timeout) {
  bool result = f();
  while (!result && timeout > 0s) {
    std::this_thread::sleep_for(1s);
    timeout -= 1s;
    result = f();
  }
  return result;
}

template<class F>
bool sleep_until(F f) {
  return sleep_until(f, 100s);
}


TEST(INTEGRATION, connection_reconfig) {
  // Test that new nodes can form a graph even if first node are all
  // disconnected.
  std::cout << "========================================" << std::endl;
  BotTest bot0("bot0.json");
  std::cout << "++++++++++++++++++++++++++++++++++++++++" << std::endl;
  BotTest bot1("bot1.json");
  std::this_thread::sleep_for(200ms);
  BotTest bot2("bot2.json");
  std::this_thread::sleep_for(200ms);
  BotTest bot3("integration_propagation40.json");

  std::this_thread::sleep_for(200ms);

  ASSERT_TRUE(bot0.check_peers_ports({1338, 1339, 13340}));
  ASSERT_TRUE(bot1.check_peers_ports({1337, 1339, 13340}));
  ASSERT_TRUE(bot2.check_peers_ports({1337, 1338, 13340}));
  ASSERT_TRUE(bot3.check_peers_ports({1337, 1338, 1339}));

  // Now adding peers that know only onebot of the network.
  BotTest bot4("integration_propagation50.json");
  std::this_thread::sleep_for(200ms);
  BotTest bot5("integration_propagation51.json");
  std::this_thread::sleep_for(200ms);
  BotTest bot6("integration_propagation52.json");

  std::this_thread::sleep_for(200ms);

  // Check there is no change for current network.
  ASSERT_TRUE(bot0.check_peers_ports({1338, 1339, 13340}));
  ASSERT_TRUE(bot1.check_peers_ports({1337, 1339, 13340}));
  ASSERT_TRUE(bot2.check_peers_ports({1337, 1338, 13340}));
  ASSERT_TRUE(bot3.check_peers_ports({1337, 1338, 1339}));
  {
    std::ofstream my_logs("my_logs_reconfig-1");
    my_logs << "37 peerss " << std::endl << bot0->peers() << std::endl;
    my_logs << "50 peerss " << std::endl << bot4->peers() << std::endl;
    my_logs << "51 peerss " << std::endl << bot5->peers() << std::endl;
    my_logs << "52 peerss " << std::endl << bot6->peers() << std::endl;
    my_logs.close();
  }

  ASSERT_TRUE(bot4.check_peers_ports({13351, 13352}));
  ASSERT_TRUE(bot5.check_peers_ports({13350, 13352}));
  ASSERT_TRUE(bot6.check_peers_ports({13350, 13351}));

  
  // Delete 3 first bots.
  bot1.operator->().reset();
  bot2.operator->().reset();
  bot3.operator->().reset();
  std::this_thread::sleep_for(1s);

  ASSERT_TRUE(bot0.check_peer_status_by_port(13350, messages::Peer::UNREACHABLE));
  ASSERT_TRUE(bot0.check_peer_status_by_port(13351, messages::Peer::UNREACHABLE));
  ASSERT_TRUE(bot0.check_peer_status_by_port(13352, messages::Peer::UNREACHABLE));
  ASSERT_TRUE(bot0.check_peers_ports({})) << std::time(nullptr);

  
  std::this_thread::sleep_for(10s);
  {
    std::ofstream my_logs("my_logs_reconfig");
    my_logs << "37 peerss " << std::endl << bot0->peers() << std::endl;
    my_logs << "50 peerss " << std::endl << bot4->peers() << std::endl;
    my_logs << "51 peerss " << std::endl << bot5->peers() << std::endl;
    my_logs << "52 peerss " << std::endl << bot6->peers() << std::endl;
    my_logs.close();
  }
  ASSERT_TRUE(TRAXERT_EQ(bot0.check_peers_ports({13350, 13351, 13352}), true)) << std::time(nullptr);

  std::cout << __LINE__ << "trax"  << std::endl;
  {
    std::ofstream my_logs("my_logs_reconfig0");
    my_logs << "37 peerss " << std::endl << bot0->peers() << std::endl;
    my_logs << "50 peerss " << std::endl << bot4->peers() << std::endl;
    my_logs << "51 peerss " << std::endl << bot5->peers() << std::endl;
    my_logs << "52 peerss " << std::endl << bot6->peers() << std::endl;
    my_logs.close();
  }
  //0x619000041580

  ASSERT_TRUE(bot4.check_peers_ports({1337, 13351, 13352}));
  ASSERT_TRUE(bot5.check_peers_ports({1337, 13350, 13352}));
  ASSERT_TRUE(bot6.check_peers_ports({1337, 13350, 13351}));

  std::this_thread::sleep_for(13s);

  {
    std::ofstream my_logs("my_logs_reconfig1");
    my_logs << "37 peerss " << std::endl << bot0->peers() << std::endl;
    my_logs << "50 peerss " << std::endl << bot4->peers() << std::endl;
    my_logs << "51 peerss " << std::endl << bot5->peers() << std::endl;
    my_logs << "52 peerss " << std::endl << bot6->peers() << std::endl;
    my_logs.close();
  }
  
  // Check we get a new fully connected network.
}
}  // namespace tests

}  // namespace neuro
