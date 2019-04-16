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

    auto has_status = (*remote)->status() & status;
    EXPECT_TRUE(has_status) << (*remote)->status();
    return has_status;
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

TEST(INTEGRATION, full_node) {
  // Try to connect to a bot that is full. The full node should me marked as
  // UNREACHABLE and the node that initiated the connection should be marked as
  // UNREACHABLE too
  BotTest bot0("bot0.json");
  bot0.set_max_incoming_connections(0);
  std::this_thread::sleep_for(5s);
  BotTest bot1("bot1.json");
  std::this_thread::sleep_for(5s);

  ASSERT_EQ(bot0.peers().size(), 2);
  ASSERT_EQ(bot0->connected_peers().size(), 0);
  ASSERT_EQ(bot1.peers().size(), 2);
  ASSERT_EQ(bot1->connected_peers().size(), 0);

  auto peer0 = bot0.peers().find(bot1.me().key_pub());
  auto peer1 = bot1.peers().find(bot0.me().key_pub());
  ASSERT_TRUE(peer0);
  ASSERT_TRUE(peer1);
  ASSERT_EQ((*peer0)->status(), messages::Peer::UNREACHABLE);
  ASSERT_EQ((*peer1)->status(), messages::Peer::UNREACHABLE);
}

TEST(INTEGRATION, simple_interaction) {
  int received_connection = 0;
  int received_deconnection = 0;
  Path config_path0("bot0.json");
  messages::config::Config config0(config_path0);
  auto bot0 = std::make_shared<Bot>(config0);
  bot0->subscribe(
      messages::Type::kConnectionReady,
      [&](const messages::Header &header, const messages::Body &body) {
        received_connection++;
      });

  Path config_path1("bot1.json");
  messages::config::Config config1(config_path1);
  auto bot1 = std::make_shared<Bot>(config1);
  bot1->subscribe(
      messages::Type::kConnectionReady,
      [&](const messages::Header &header, const messages::Body &body) {
        received_connection++;
      });

  Path config_path2("bot2.json");
  messages::config::Config config2(config_path2);
  auto bot2 = std::make_shared<Bot>(config2);
  bot2->subscribe(
      messages::Type::kConnectionReady,
      [&](const messages::Header &header, const messages::Body &body) {
        received_connection++;
      });

  std::this_thread::sleep_for(1s);

  ASSERT_GT(received_connection, 0);
  ASSERT_EQ(received_deconnection, 0);
  std::this_thread::sleep_for(std::chrono::milliseconds(1000));

  std::this_thread::sleep_for(1s);
  auto peers_bot0 = (bot0->connected_peers());
  auto peers_bot1 = (bot1->connected_peers());
  auto peers_bot2 = (bot2->connected_peers());
  std::ofstream my_logs("my_logs");
  my_logs << "0 peerss " << std::endl << bot0->peers() << std::endl;
  my_logs << "1 peerss " << std::endl << bot1->peers() << std::endl;
  my_logs << "2 peerss " << std::endl << bot2->peers() << std::endl;
  my_logs.close();

  EXPECT_EQ(peers_bot0.size(), peers_bot1.size());
  EXPECT_EQ(peers_bot1.size(), peers_bot2.size());

  ASSERT_EQ(peers_bot2.size(), 2);

  ASSERT_EQ(peers_bot0[0]->endpoint(), "localhost");
  ASSERT_EQ(peers_bot0[1]->endpoint(), "localhost");
  ASSERT_TRUE(peers_bot0[0]->port() == 1338 || peers_bot0[0]->port() == 1339);
  ASSERT_TRUE(peers_bot0[1]->port() == 1338 || peers_bot0[1]->port() == 1339);
  ASSERT_NE(peers_bot0[0]->port(), peers_bot0[1]->port());

  ASSERT_EQ(peers_bot1[0]->endpoint(), "localhost");
  ASSERT_EQ(peers_bot1[1]->endpoint(), "localhost");
  ASSERT_TRUE(peers_bot1[0]->port() == 1337 || peers_bot1[0]->port() == 1339);
  ASSERT_TRUE(peers_bot1[1]->port() == 1337 || peers_bot1[1]->port() == 1339);
  ASSERT_NE(peers_bot1[0]->port(), peers_bot1[1]->port());

  ASSERT_EQ(peers_bot2[0]->endpoint(), "localhost");
  ASSERT_EQ(peers_bot2[1]->endpoint(), "localhost");
  ASSERT_TRUE(peers_bot2[0]->port() == 1337 || peers_bot2[0]->port() == 1338);
  ASSERT_TRUE(peers_bot2[1]->port() == 1337 || peers_bot2[1]->port() == 1338);
  ASSERT_NE(peers_bot2[0]->port(), peers_bot2[1]->port());

  bot2.reset();
  std::this_thread::sleep_for(1s);
  bot1.reset();
  bot0.reset();
  LOG_TRACE << "the end?" << std::endl;
}

TEST(INTEGRATION, disconnect) {
  BotTest bot0("bot0.json");
  BotTest bot1("bot1.json");

  std::this_thread::sleep_for(1s);

  ASSERT_EQ(bot0->connected_peers().size(), 1);
  ASSERT_EQ(bot1->connected_peers().size(), 1);

  bot1.operator->().reset();

  std::this_thread::sleep_for(10s);

  ASSERT_EQ(bot0->connected_peers().size(), 0);
}

TEST(INTEGRATION, neighbors_propagation) {
  BotTest bot0("integration_propagation0.json");
  BotTest bot1("integration_propagation1.json");
  BotTest bot2("integration_propagation2.json");
  std::this_thread::sleep_for(2s);

  ASSERT_TRUE(bot0.check_peers_ports({1338, 1339}));
  ASSERT_TRUE(bot1.check_peers_ports({1337, 1339}));
  ASSERT_TRUE(bot2.check_peers_ports({1337, 1338}));
}

TEST(INTEGRATION, neighbors_connections_with_delays) {
  Path config_path0("bot0.json");
  messages::config::Config config0(config_path0);
  auto bot0 = std::make_shared<Bot>(config0);
  std::this_thread::sleep_for(5s);
  Path config_path1("bot1.json");
  messages::config::Config config1(config_path1);
  auto bot1 = std::make_shared<Bot>(config1);
  Path config_path2("bot2.json");
  messages::config::Config config2(config_path2);
  auto bot2 = std::make_shared<Bot>(config2);

  std::this_thread::sleep_for(5s);

  auto peers_bot0 = vectorize(bot0->connected_peers());
  auto peers_bot1 = vectorize(bot1->connected_peers());
  auto peers_bot2 = vectorize(bot2->connected_peers());

  ASSERT_EQ(peers_bot0.size(), peers_bot1.size());
  ASSERT_EQ(peers_bot1.size(), peers_bot2.size());
  ASSERT_EQ(peers_bot2.size(), 2);

  ASSERT_EQ(peers_bot0[0]->endpoint(), "localhost");
  ASSERT_EQ(peers_bot0[1]->endpoint(), "localhost");
  ASSERT_TRUE(peers_bot0[0]->port() == 1338 || peers_bot0[0]->port() == 1339);
  ASSERT_TRUE(peers_bot0[1]->port() == 1338 || peers_bot0[1]->port() == 1339);
  ASSERT_NE(peers_bot0[0]->port(), peers_bot0[1]->port());

  ASSERT_EQ(peers_bot1[0]->endpoint(), "localhost");
  ASSERT_EQ(peers_bot1[1]->endpoint(), "localhost");
  ASSERT_TRUE(peers_bot1[0]->port() == 1337 || peers_bot1[0]->port() == 1339);
  ASSERT_TRUE(peers_bot1[1]->port() == 1337 || peers_bot1[1]->port() == 1339);
  ASSERT_NE(peers_bot1[0]->port(), peers_bot1[1]->port());

  ASSERT_EQ(peers_bot2[0]->endpoint(), "localhost");
  ASSERT_EQ(peers_bot2[1]->endpoint(), "localhost");
  ASSERT_TRUE(peers_bot2[0]->port() == 1337 || peers_bot2[0]->port() == 1338);
  ASSERT_TRUE(peers_bot2[1]->port() == 1337 || peers_bot2[1]->port() == 1338);
  ASSERT_NE(peers_bot2[0]->port(), peers_bot2[1]->port());
}

TEST(INTEGRATION, neighbors_update) {
  Path config_path0("integration_propagation0.json");
  messages::config::Config config0(config_path0);
  auto bot0 = std::make_shared<Bot>(config0);
  Path config_path1("integration_propagation1.json");
  messages::config::Config config1(config_path1);
  auto bot1 = std::make_shared<Bot>(config1);
  Path config_path2("integration_propagation2.json");
  messages::config::Config config2(config_path2);
  auto bot2 = std::make_shared<Bot>(config2);

  std::this_thread::sleep_for(5s);

  auto peers_bot0 = vectorize(bot0->connected_peers());
  auto peers_bot1 = vectorize(bot1->connected_peers());
  auto peers_bot2 = vectorize(bot2->connected_peers());

  ASSERT_EQ(peers_bot0.size(), peers_bot1.size());
  ASSERT_EQ(peers_bot1.size(), peers_bot2.size());
  ASSERT_EQ(peers_bot2.size(), 2);

  ASSERT_EQ(peers_bot0[0]->endpoint(), "localhost");
  ASSERT_EQ(peers_bot0[1]->endpoint(), "localhost");
  ASSERT_TRUE(peers_bot0[0]->port() == 1338 || peers_bot0[0]->port() == 1339);
  ASSERT_TRUE(peers_bot0[1]->port() == 1338 || peers_bot0[1]->port() == 1339);
  ASSERT_NE(peers_bot0[0]->port(), peers_bot0[1]->port());

  ASSERT_EQ(peers_bot1[0]->endpoint(), "localhost");
  ASSERT_EQ(peers_bot1[1]->endpoint(), "localhost");
  ASSERT_TRUE(peers_bot1[0]->port() == 1337 || peers_bot1[0]->port() == 1339);
  ASSERT_TRUE(peers_bot1[1]->port() == 1337 || peers_bot1[1]->port() == 1339);
  ASSERT_NE(peers_bot1[0]->port(), peers_bot1[1]->port());

  ASSERT_EQ(peers_bot2[0]->endpoint(), "localhost");
  ASSERT_EQ(peers_bot2[1]->endpoint(), "localhost");
  ASSERT_TRUE(peers_bot2[0]->port() == 1337 || peers_bot2[0]->port() == 1338);
  ASSERT_TRUE(peers_bot2[1]->port() == 1337 || peers_bot2[1]->port() == 1338);
  ASSERT_NE(peers_bot2[0]->port(), peers_bot2[1]->port());
}

TEST(INTEGRATION, key_gen_connection) {
  // Check an unknown bot with unknown signature can connect to bot0.
  Path config_path0("bot0.json");
  messages::config::Config config0(config_path0);
  auto bot0 = std::make_shared<Bot>(config0);

  Path config_path1("integration_propagation50.json");
  messages::config::Config config1(config_path1);
  auto bot50 = std::make_shared<Bot>(config1);

  std::this_thread::sleep_for(5s);

  auto peers_bot0 = vectorize(bot0->connected_peers());
  auto peers_bot50 = vectorize(bot50->connected_peers());

  ASSERT_EQ(peers_bot0.size(), 1);
  ASSERT_EQ(peers_bot50.size(), 1);

  std::ofstream my_logs("my_logs_gen_conn");
  my_logs << "0  peerss \n" << bot0->peers() << std::endl;
  my_logs << "50 peerss \n" << bot50->peers() << std::endl;
  my_logs.close();

  ASSERT_EQ(peers_bot0[0]->endpoint(), "localhost");
  ASSERT_EQ(peers_bot0[0]->port(), 13350);

  ASSERT_EQ(peers_bot50[0]->endpoint(), "localhost");
  ASSERT_EQ(peers_bot50[0]->port(), 1337);
}

TEST(INTEGRATION, fullfill_network) {
  // Add a fourth node to get a complete graph
  Path config_path0("bot0.json");
  messages::config::Config config0(config_path0);
  auto bot0 = std::make_shared<Bot>(config0);
  Path config_path1("bot1.json");
  messages::config::Config config1(config_path1);
  auto bot1 = std::make_shared<Bot>(config1);
  Path config_path2("bot2.json");
  messages::config::Config config2(config_path2);
  auto bot2 = std::make_shared<Bot>(config2);

  std::this_thread::sleep_for(5s);

  // Now adding last node
  Path config_path40("integration_propagation40.json");
  messages::config::Config config40(config_path40);
  auto bot40 = std::make_shared<Bot>(config40);

  std::this_thread::sleep_for(15s);

  auto peers_bot0 = vectorize(bot0->connected_peers());
  auto peers_bot1 = vectorize(bot1->connected_peers());
  auto peers_bot2 = vectorize(bot2->connected_peers());
  auto peers_bot40 = vectorize(bot40->connected_peers());

  std::ofstream my_logs("my_logs_fullfill");
  my_logs << "0  peerss \n" << bot0->peers() << std::endl;
  my_logs << "1  peerss \n" << bot1->peers() << std::endl;
  my_logs << "2  peerss \n" << bot2->peers() << std::endl;
  my_logs << "40 peerss \n" << bot40->peers() << std::endl;
  my_logs.close();

  ASSERT_EQ(peers_bot0.size(), peers_bot1.size());
  ASSERT_EQ(peers_bot1.size(), peers_bot2.size());
  ASSERT_EQ(peers_bot2.size(), peers_bot40.size());
  ASSERT_EQ(peers_bot40.size(), 3);

  ASSERT_EQ(peers_bot0[0]->endpoint(), "localhost");
  ASSERT_EQ(peers_bot0[1]->endpoint(), "localhost");
  ASSERT_EQ(peers_bot0[2]->endpoint(), "localhost");
  ASSERT_TRUE(peers_bot0[0]->port() == 1338 || peers_bot0[0]->port() == 1339 ||
              peers_bot0[0]->port() == 13340);
  ASSERT_TRUE(peers_bot0[1]->port() == 1338 || peers_bot0[1]->port() == 1339 ||
              peers_bot0[1]->port() == 13340);
  ASSERT_TRUE(peers_bot0[2]->port() == 1338 || peers_bot0[2]->port() == 1339 ||
              peers_bot0[2]->port() == 13340);
  ASSERT_NE(peers_bot0[0]->port(), peers_bot0[1]->port());
  ASSERT_NE(peers_bot0[1]->port(), peers_bot0[2]->port());
  ASSERT_NE(peers_bot0[0]->port(), peers_bot0[2]->port());

  ASSERT_EQ(peers_bot1[0]->endpoint(), "localhost");
  ASSERT_EQ(peers_bot1[1]->endpoint(), "localhost");
  ASSERT_EQ(peers_bot1[2]->endpoint(), "localhost");
  ASSERT_TRUE(peers_bot1[0]->port() == 1337 || peers_bot1[0]->port() == 1339 ||
              peers_bot1[0]->port() == 13340);
  ASSERT_TRUE(peers_bot1[1]->port() == 1337 || peers_bot1[1]->port() == 1339 ||
              peers_bot1[1]->port() == 13340);
  ASSERT_TRUE(peers_bot1[2]->port() == 1337 || peers_bot1[2]->port() == 1339 ||
              peers_bot1[2]->port() == 13340);
  ASSERT_NE(peers_bot1[0]->port(), peers_bot1[1]->port());
  ASSERT_NE(peers_bot1[1]->port(), peers_bot1[2]->port());
  ASSERT_NE(peers_bot1[0]->port(), peers_bot1[2]->port());

  ASSERT_EQ(peers_bot2[0]->endpoint(), "localhost");
  ASSERT_EQ(peers_bot2[1]->endpoint(), "localhost");
  ASSERT_EQ(peers_bot2[2]->endpoint(), "localhost");
  ASSERT_TRUE(peers_bot2[0]->port() == 1337 || peers_bot2[0]->port() == 1338 ||
              peers_bot2[0]->port() == 13340);
  ASSERT_TRUE(peers_bot2[1]->port() == 1337 || peers_bot2[1]->port() == 1338 ||
              peers_bot2[1]->port() == 13340);
  ASSERT_TRUE(peers_bot2[2]->port() == 1337 || peers_bot2[2]->port() == 1338 ||
              peers_bot2[2]->port() == 13340);
  ASSERT_NE(peers_bot2[0]->port(), peers_bot2[1]->port());
  ASSERT_NE(peers_bot2[1]->port(), peers_bot2[2]->port());
  ASSERT_NE(peers_bot2[0]->port(), peers_bot2[2]->port());

  ASSERT_EQ(peers_bot40[0]->endpoint(), "localhost");
  ASSERT_EQ(peers_bot40[1]->endpoint(), "localhost");
  ASSERT_EQ(peers_bot40[2]->endpoint(), "localhost");
  ASSERT_TRUE(peers_bot40[0]->port() == 1337 ||
              peers_bot40[0]->port() == 1338 || peers_bot40[0]->port() == 1339);
  ASSERT_TRUE(peers_bot40[1]->port() == 1337 ||
              peers_bot40[1]->port() == 1338 || peers_bot40[1]->port() == 1339);
  ASSERT_TRUE(peers_bot40[2]->port() == 1337 ||
              peers_bot40[2]->port() == 1338 || peers_bot40[2]->port() == 1339);
  ASSERT_NE(peers_bot40[0]->port(), peers_bot40[1]->port());
  ASSERT_NE(peers_bot40[1]->port(), peers_bot40[2]->port());
  ASSERT_NE(peers_bot40[0]->port(), peers_bot40[2]->port());
}

TEST(INTEGRATION, connection_opportunity) {
  // Check a node excluded from a connected graph can connect after a node of
  // the complete graph is destroyed.
  BotTest bot0("bot0.json");
  BotTest bot1("bot1.json");
  BotTest bot2("bot2.json");
  BotTest bot3("integration_propagation40.json");
  std::this_thread::sleep_for(2s);

  ASSERT_TRUE(bot0.check_peers_ports({1338, 1339, 13340}));
  ASSERT_TRUE(bot1.check_peers_ports({1337, 1339, 13340}));
  ASSERT_TRUE(bot2.check_peers_ports({1337, 1338, 13340}));
  ASSERT_TRUE(bot3.check_peers_ports({1337, 1338, 1339}));

  // Create additional node that cannot connect
  BotTest bot4("integration_propagation41.json");
  std::this_thread::sleep_for(2s);

  ASSERT_TRUE(bot0.check_peers_ports({1338, 1339, 13340})) << bot0->peers();
  ASSERT_TRUE(bot1.check_peers_ports({1337, 1339, 13340})) << bot1->peers();
  ASSERT_TRUE(bot2.check_peers_ports({1337, 1338, 13340})) << bot2->peers();
  ASSERT_TRUE(bot3.check_peers_ports({1337, 1338, 1339})) << bot3->peers();

  // Check no change on other nodes
  ASSERT_TRUE(bot4.check_peers_ports({}));

  // Destroy a connected bot.
  bot3.operator->().reset();
  std::this_thread::sleep_for(15s);
  ASSERT_TRUE(bot0.check_peers_ports({1338, 1339, 13341})) << bot0->peers();
  ASSERT_TRUE(bot1.check_peers_ports({1337, 1339, 13341})) << bot1->peers();
  ASSERT_TRUE(bot2.check_peers_ports({1337, 1338, 13341})) << bot2->peers();
  ASSERT_TRUE(bot4.check_peers_ports({1337, 1338, 1339})) << bot4->peers();
}

TEST(INTEGRATION, connection_opportunity_update) {
  // Check a node excluded from a connected graph can connect after a node of
  // the complete graph accept one more connection.
  BotTest bot0("bot0.json");
  BotTest bot1("bot1.json");
  BotTest bot2("bot2.json");
  BotTest bot3("integration_propagation40.json");
  std::this_thread::sleep_for(2s);

  ASSERT_TRUE(bot0.check_peers_ports({1338, 1339, 13340}));
  ASSERT_TRUE(bot1.check_peers_ports({1337, 1339, 13340}));
  ASSERT_TRUE(bot2.check_peers_ports({1337, 1338, 13340}));
  ASSERT_TRUE(bot3.check_peers_ports({1337, 1338, 1339}));

  // Create additional node that cannot connect
  BotTest bot4("integration_propagation50.json");
  std::this_thread::sleep_for(2s);

  // Check no change on other nodes
  ASSERT_TRUE(bot0.check_peers_ports({1338, 1339, 13340}));
  ASSERT_TRUE(bot1.check_peers_ports({1337, 1339, 13340}));
  ASSERT_TRUE(bot2.check_peers_ports({1337, 1338, 13340}));
  ASSERT_TRUE(bot3.check_peers_ports({1337, 1338, 1339}));
  ASSERT_TRUE(bot4.check_peers_ports({}));

  // Make bot3 accept one more connection
  bot3.set_max_incoming_connections(4);
  std::this_thread::sleep_for(20s);
  ASSERT_TRUE(bot0.check_peers_ports({1338, 1339, 13340}));
  ASSERT_TRUE(bot1.check_peers_ports({1337, 1339, 13340}));
  ASSERT_TRUE(bot2.check_peers_ports({1337, 1338, 13340}));
  ASSERT_TRUE(bot3.check_peers_ports({1337, 1338, 1339, 13350}));
  ASSERT_TRUE(bot4.check_peers_ports({13340}));
}

TEST(INTEGRATION, connection_reconfig) {
  // Test that new nodes can form a graph even if first node are all
  // disconnected.
  BotTest bot0("bot0.json");
  BotTest bot1("bot1.json");
  BotTest bot2("bot2.json");
  BotTest bot3("integration_propagation40.json");
  std::this_thread::sleep_for(1s);

  ASSERT_TRUE(bot0.check_peers_ports({1338, 1339, 13340})) << bot0->peers();
  ASSERT_TRUE(bot1.check_peers_ports({1337, 1339, 13340})) << bot1->peers();
  ASSERT_TRUE(bot2.check_peers_ports({1337, 1338, 13340})) << bot2->peers();
  ASSERT_TRUE(bot3.check_peers_ports({1337, 1338, 1339})) << bot3->peers();

  // Now adding peers that know only onebot of the network.
  std::this_thread::sleep_for(3s);
  BotTest bot4("integration_propagation50.json");
  BotTest bot5("integration_propagation51.json");
  BotTest bot6("integration_propagation52.json");

  // give them more time, they need to ask bot0 for more peer
  std::this_thread::sleep_for(7s);

  // Check there is no change for current network.
  ASSERT_TRUE(bot0.check_peers_ports({1338, 1339, 13340})) << bot0->peers();
  ASSERT_TRUE(bot1.check_peers_ports({1337, 1339, 13340})) << bot1->peers();
  ASSERT_TRUE(bot2.check_peers_ports({1337, 1338, 13340})) << bot2->peers();
  ASSERT_TRUE(bot3.check_peers_ports({1337, 1338, 1339})) << bot3->peers();
  ASSERT_TRUE(bot4.check_peers_ports({13351, 13352})) << bot4->peers();
  ASSERT_TRUE(bot5.check_peers_ports({13350, 13352})) << bot5->peers();
  ASSERT_TRUE(bot6.check_peers_ports({13350, 13351})) << bot6->peers();

  const auto unavailable = static_cast<messages::Peer::Status>
      (messages::Peer::UNREACHABLE | messages::Peer::DISCONNECTED);
  ASSERT_TRUE(bot0.check_peer_status_by_port(13350, unavailable));
  ASSERT_TRUE(bot0.check_peer_status_by_port(13351, unavailable));
  ASSERT_TRUE(bot0.check_peer_status_by_port(13352, unavailable));

  // Delete 3 first bots.
  bot1.operator->().reset();
  bot2.operator->().reset();
  bot3.operator->().reset();
  std::this_thread::sleep_for(10s);

  ASSERT_TRUE(bot0.check_peers_ports({13350, 13351, 13352})) << bot0->peers();
  ASSERT_TRUE(bot4.check_peers_ports({1337, 13351, 13352})) << bot4->peers();
  ASSERT_TRUE(bot5.check_peers_ports({1337, 13350, 13352})) << bot5->peers();
  ASSERT_TRUE(bot6.check_peers_ports({1337, 13350, 13351})) << bot6->peers();
}

TEST(INTEGRATION, ignore_bad_message) {
  BotTest bot0("bot0.json");
  BotTest bot1("bot1.json");

  std::this_thread::sleep_for(5s);

  ASSERT_TRUE(bot0.check_peers_ports({1338}));
  ASSERT_TRUE(bot1.check_peers_ports({1337}));

  bot1->subscribe(messages::Type::kGetPeers,
                  [](const messages::Header &header,
                     const messages::Body &body) {
                    EXPECT_EQ(header.version(), neuro::MessageVersion);
                  });

  auto msg = std::make_shared<messages::Message>();
  auto *header = msg->mutable_header();
  messages::fill_header(header);
  msg->add_bodies()->mutable_get_peers();
  header->set_version(neuro::MessageVersion + 100);
  bot0.networking().send(msg);

  std::this_thread::sleep_for(5s);
}

TEST(INTEGRATION, keep_max_connections) {
  BotTest bot0("bot0.json");
  bot0.set_max_incoming_connections(1);
  std::this_thread::sleep_for(1s);
  BotTest bot1("bot1.json");
  std::this_thread::sleep_for(1s);
  BotTest bot2("bot2.json");
  std::this_thread::sleep_for(20s);

  ASSERT_TRUE(bot0.check_peers_ports({1338}));
  ASSERT_TRUE(bot1.check_peers_ports({1337, 1339}));
  ASSERT_TRUE(bot2.check_peers_ports({1338}));
}
}  // namespace tests

}  // namespace neuro
