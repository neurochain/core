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

namespace neuro::tests {

using namespace std::chrono_literals;

class BotTest {
 private:
  std::unique_ptr<neuro::Bot> _bot;
  Port _port_offset;

  void apply_port_offset(messages::config::Config& config) {
    auto port = config.networking().tcp().port();
    messages::config::Tcp* tcp_config =
        config.mutable_networking()->mutable_tcp();
    tcp_config->set_port(port + _port_offset);
    for (auto& peer : *tcp_config->mutable_peers()) {
      auto peer_port = peer.port();
      peer.set_port(peer_port + _port_offset);
    }
  }

 public:
  explicit BotTest(const std::string configPath,
          const Port port_offset = 0) : _port_offset(port_offset) {
    Path configAsPath(configPath);
    messages::config::Config config(configAsPath);
    apply_port_offset(config);
    _bot = std::make_unique<Bot>(config);
    _bot->_max_incoming_connections = 3;
    _bot->_max_connections = 3;
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
      peers_ports.push_back(peer->port() - _port_offset);
    }
    std::sort(peers_ports.begin(), peers_ports.end());
    EXPECT_EQ(ports, peers_ports);
    return peers_ports == ports;
  }

  bool check_peer_status_by_port(const Port port,
				 const messages::Peer::Status status) {
    const auto remote = _bot->peers().peer_by_port(port + _port_offset);
    EXPECT_TRUE(remote) << _bot->peers();
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
  Port port_offset = 1000;
  BotTest bot0("bot0.json", port_offset);
  bot0.set_max_incoming_connections(0);
  std::this_thread::sleep_for(5s);
  BotTest bot1("bot1.json", port_offset);
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
  Port port_offset = 2000;
  BotTest bot0("bot0.json", port_offset);
  bot0->subscribe(
      messages::Type::kConnectionReady,
      [&](const messages::Header &header, const messages::Body &body) {
        received_connection++;
      });

  BotTest bot1("bot1.json", port_offset);
  bot1->subscribe(
      messages::Type::kConnectionReady,
      [&](const messages::Header &header, const messages::Body &body) {
        received_connection++;
      });

  BotTest bot2("bot2.json", port_offset);
  bot2->subscribe(
      messages::Type::kConnectionReady,
      [&](const messages::Header &header, const messages::Body &body) {
        received_connection++;
      });

  std::this_thread::sleep_for(1s);

  ASSERT_GT(received_connection, 0);
  ASSERT_EQ(received_deconnection, 0);

  std::this_thread::sleep_for(2s);

  ASSERT_TRUE(bot0.check_peers_ports({1338, 1339})) << bot0->peers();
  ASSERT_TRUE(bot1.check_peers_ports({1337, 1339})) << bot1->peers();
  ASSERT_TRUE(bot2.check_peers_ports({1337, 1338})) << bot2->peers();

  bot2.operator->().reset();
  std::this_thread::sleep_for(1s);
  bot1.operator->().reset();
  bot0.operator->().reset();
}

TEST(INTEGRATION, disconnect) {
  Port port_offset = 3000;
  BotTest bot0("bot0.json", port_offset);
  BotTest bot1("bot1.json", port_offset);

  std::this_thread::sleep_for(1s);

  ASSERT_EQ(bot0->connected_peers().size(), 1);
  ASSERT_EQ(bot1->connected_peers().size(), 1);

  bot1.operator->().reset();

  std::this_thread::sleep_for(10s);

  ASSERT_EQ(bot0->connected_peers().size(), 0);
}

TEST(INTEGRATION, neighbors_propagation) {
  Port port_offset = 4000;
  BotTest bot0("integration_propagation0.json", port_offset);
  BotTest bot1("integration_propagation1.json", port_offset);
  BotTest bot2("integration_propagation2.json", port_offset);
  std::this_thread::sleep_for(2s);

  ASSERT_TRUE(bot0.check_peers_ports({1338, 1339})) << bot0->peers();
  ASSERT_TRUE(bot1.check_peers_ports({1337, 1339})) << bot1->peers();
  ASSERT_TRUE(bot2.check_peers_ports({1337, 1338})) << bot2->peers();
}

TEST(INTEGRATION, neighbors_connections_with_delays) {
  Port port_offset = 5000;
  BotTest bot0("bot0.json", port_offset);
  BotTest bot1("bot1.json", port_offset);
  BotTest bot2("bot2.json", port_offset);

  std::this_thread::sleep_for(5s);

  ASSERT_TRUE(bot0.check_peers_ports({1338, 1339})) << bot0->peers();
  ASSERT_TRUE(bot1.check_peers_ports({1337, 1339})) << bot1->peers();
  ASSERT_TRUE(bot2.check_peers_ports({1337, 1338})) << bot2->peers();
}

TEST(INTEGRATION, neighbors_update) {
  Port port_offset = 6000;
  BotTest bot0("integration_propagation0.json", port_offset);
  BotTest bot1("integration_propagation1.json", port_offset);
  BotTest bot2("integration_propagation2.json", port_offset);

  std::this_thread::sleep_for(12s);

  ASSERT_TRUE(bot0.check_peers_ports({1338, 1339})) << bot0->peers();
  ASSERT_TRUE(bot1.check_peers_ports({1337, 1339})) << bot1->peers();
  ASSERT_TRUE(bot2.check_peers_ports({1337, 1338})) << bot2->peers();
}

TEST(INTEGRATION, key_gen_connection) {
  // Check an unknown bot with unknown signature can connect to bot0.
  Port port_offset = 7000;
  BotTest bot0("bot0.json", port_offset);
  BotTest bot50("integration_propagation50.json", port_offset);

  std::this_thread::sleep_for(5s);

  ASSERT_TRUE(bot0.check_peers_ports({13350})) << bot0->peers();
  ASSERT_TRUE(bot50.check_peers_ports({1337})) << bot50->peers();
}

TEST(INTEGRATION, fullfill_network) {
  // Add a fourth node to get a complete graph
  Port port_offset = 8000;
  BotTest bot0("bot0.json", port_offset);
  BotTest bot1("bot1.json", port_offset);
  BotTest bot2("bot2.json", port_offset);

  std::this_thread::sleep_for(5s);

  // Now adding last node
  BotTest bot40("integration_propagation40.json", port_offset);

  std::this_thread::sleep_for(15s);

  ASSERT_TRUE(bot0.check_peers_ports({1338, 1339, 13340})) << bot0->peers();
  ASSERT_TRUE(bot1.check_peers_ports({1337, 1339, 13340})) << bot1->peers();
  ASSERT_TRUE(bot2.check_peers_ports({1337, 1338, 13340})) << bot2->peers();
  ASSERT_TRUE(bot40.check_peers_ports({1337, 1338, 1339})) << bot40->peers();
}

TEST(INTEGRATION, connection_opportunity) {
  // Check a node excluded from a connected graph can connect after a node of
  // the complete graph is destroyed.
  Port port_offset = 9000;
  BotTest bot0("bot0.json", port_offset);
  BotTest bot1("bot1.json", port_offset);
  BotTest bot2("bot2.json", port_offset);
  BotTest bot3("integration_propagation40.json", port_offset);
  std::this_thread::sleep_for(2s);

  ASSERT_TRUE(bot0.check_peers_ports({1338, 1339, 13340}));
  ASSERT_TRUE(bot1.check_peers_ports({1337, 1339, 13340}));
  ASSERT_TRUE(bot2.check_peers_ports({1337, 1338, 13340}));
  ASSERT_TRUE(bot3.check_peers_ports({1337, 1338, 1339}));

  // Create additional node that cannot connect
  BotTest bot4("integration_propagation41.json", port_offset);
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
  Port port_offset = 10000;
  BotTest bot0("bot0.json", port_offset);
  BotTest bot1("bot1.json", port_offset);
  BotTest bot2("bot2.json", port_offset);
  BotTest bot3("integration_propagation40.json", port_offset);
  std::this_thread::sleep_for(2s);

  ASSERT_TRUE(bot0.check_peers_ports({1338, 1339, 13340}));
  ASSERT_TRUE(bot1.check_peers_ports({1337, 1339, 13340}));
  ASSERT_TRUE(bot2.check_peers_ports({1337, 1338, 13340}));
  ASSERT_TRUE(bot3.check_peers_ports({1337, 1338, 1339}));

  // Create additional node that cannot connect
  BotTest bot4("integration_propagation50.json", port_offset);
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
  Port port_offset = 11000;
  BotTest bot0("bot0.json", port_offset);
  BotTest bot1("bot1.json", port_offset);
  BotTest bot2("bot2.json", port_offset);
  BotTest bot3("integration_propagation40.json", port_offset);
  std::this_thread::sleep_for(1s);

  ASSERT_TRUE(bot0.check_peers_ports({1338, 1339, 13340})) << bot0->peers();
  ASSERT_TRUE(bot1.check_peers_ports({1337, 1339, 13340})) << bot1->peers();
  ASSERT_TRUE(bot2.check_peers_ports({1337, 1338, 13340})) << bot2->peers();
  ASSERT_TRUE(bot3.check_peers_ports({1337, 1338, 1339})) << bot3->peers();

  // Now adding peers that know only onebot of the network.
  std::this_thread::sleep_for(3s);
  BotTest bot4("integration_propagation50.json", port_offset);
  BotTest bot5("integration_propagation51.json", port_offset);
  BotTest bot6("integration_propagation52.json", port_offset);

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
  ASSERT_TRUE(bot0.check_peer_status_by_port(13350, unavailable)) << bot0->peers();
  ASSERT_TRUE(bot0.check_peer_status_by_port(13351, unavailable)) << bot0->peers();
  ASSERT_TRUE(bot0.check_peer_status_by_port(13352, unavailable)) << bot0->peers();

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
  Port port_offset = 12000;
  BotTest bot0("bot0.json", port_offset);
  BotTest bot1("bot1.json", port_offset);

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
  Port port_offset = 13000;
  BotTest bot0("bot0.json", port_offset);
  bot0.set_max_incoming_connections(1);
  std::this_thread::sleep_for(1s);
  BotTest bot1("bot1.json", port_offset);
  std::this_thread::sleep_for(1s);
  BotTest bot2("bot2.json", port_offset);
  std::this_thread::sleep_for(20s);

  ASSERT_TRUE(bot0.check_peers_ports({1338}));
  ASSERT_TRUE(bot1.check_peers_ports({1337, 1339}));
  ASSERT_TRUE(bot2.check_peers_ports({1338}));
}

}  // namespace neuro::tests
