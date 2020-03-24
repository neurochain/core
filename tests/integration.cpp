#include <gtest/gtest.h>
#include <chrono>
#include <sstream>
#include <thread>
#include "Bot.hpp"
#include "common/types.hpp"
#include "crypto/Sign.hpp"
#include "messages/config/Config.hpp"
#include "tooling/blockgen.hpp"

namespace neuro::tests {

using namespace std::chrono_literals;

class BotTest : public Bot {
 private:
  Port _port_offset;

  messages::config::Config as_message(const std::string &config_path) const {
    Path config_as_path(config_path);
    messages::config::Config config(config_as_path);
    return config;
  }

  void apply_port_offset() {
    auto port = _config.networking().tcp().port();
    messages::config::Tcp *tcp_config =
        _config.mutable_networking()->mutable_tcp();
    tcp_config->set_port(port + _port_offset);

    for (auto &peer : *tcp_config->mutable_peers()) {
      auto peer_port = peer.port();
      peer.set_port(peer_port + _port_offset);
    }
  }

  const consensus::Config consensus_config() {
    auto conf = consensus::Config();
    conf.blocks_per_assembly = 10;
    return conf;
  }

 public:
  explicit BotTest(const std::string config_path, const Port port_offset)
      : Bot(as_message(config_path), consensus_config()),
        _port_offset(port_offset) {
    apply_port_offset();
    _max_incoming_connections = 3;
    _max_connections = 3;
  }

  void set_max_incoming_connections(int max_incoming_connections) {
    _max_incoming_connections = max_incoming_connections;
    _max_connections = max_incoming_connections;
  }

  auto me() { return _me; }

  void update_unreachable() { _peers.update_unreachable(); }

  auto &networking() { return _networking; }

  auto &queue() { return _queue; }

  std::chrono::seconds unreachable_timeout() {
    auto t = _config.networking().default_next_update_time();
    return std::chrono::seconds{t};
  }

  bool send_all(const messages::Message &message) {
    return _networking.send_all(message) !=
           networking::TransportLayer::SendResult::FAILED;
  }

  std::chrono::seconds connecting_timeout() {
    auto t = _config.networking().connecting_next_update_time();
    return std::chrono::seconds{t};
  }

  bool check_peers_ports(std::vector<int> ports) {
    std::sort(ports.begin(), ports.end());
    std::vector<int> peers_ports;
    for (const auto &peer : connected_peers()) {
      peers_ports.push_back(peer->port() - _port_offset);
    }
    std::sort(peers_ports.begin(), peers_ports.end());
    EXPECT_EQ(ports, peers_ports);
    return peers_ports == ports;
  }

  bool check_peer_status_by_port(const Port port,
                                 const messages::Peer::Status status) {
    const auto remote = peers().peer_by_port(port + _port_offset);
    EXPECT_TRUE(remote) << peers();
    if (!remote) {
      return false;
    }

    auto has_status = (*remote)->status() & status;
    EXPECT_TRUE(has_status) << (*remote)->status();
    return has_status;
  }

  bool check_is_connected(std::vector<Port> ports) {
    for (auto port : ports) {
      std::optional<neuro::messages::Peer *> found_peer;
      for (const auto &peer : connected_peers()) {
        if (peer->port() == port + _port_offset) {
          found_peer = peer;
          break;
        }
      }
      EXPECT_TRUE(found_peer.has_value());
      if (!found_peer.has_value()) {
        return false;
      }
    }
    return true;
  }

  bool poll_connected_ports(std::vector<Port> ports,
                            std::chrono::seconds timeout = 10s) {
    bool has_ports = false;
    auto begin_polling = std::chrono::steady_clock::now();
    auto now = std::chrono::steady_clock::now();
    do {
      std::this_thread::sleep_for(200ms);
      std::sort(ports.begin(), ports.end());
      std::vector<Port> peers_ports;
      for (const auto &peer : connected_peers()) {
        peers_ports.push_back(peer->port() - _port_offset);
      }
      std::sort(peers_ports.begin(), peers_ports.end());
      has_ports = peers_ports == ports;
      now = std::chrono::steady_clock::now();
    } while (has_ports == false && (now - begin_polling < timeout));
    return has_ports;
  }

  int nb_blocks() { return _ledger->total_nb_blocks(); }

  void add_block() {
    messages::TaggedBlock new_block;
    neuro::tooling::blockgen::blockgen_from_last_db_block(
        new_block.mutable_block(), _ledger, 1, 0);
    _ledger->insert_block(new_block);
  }

  void unsubscribe() { _subscriber.unsubscribe(); }

  messages::Block block0() {
    messages::Block block0;
    _ledger->get_block(0, &block0);
    return block0;
  }
};

/**
 * generate a random port offset to launch multiple test in parallel
 * disabled by default because it can lead to "bind: Address already in use"
 * error, use gtest-parellel and uncomment to speed up the tests
 */
Port random_port() {
  Port port_offset = 0;
  //  std::random_device rd;
  //  port_offset = (rd() + 10) % 10000;
  LOG_INFO << "port offset : " << port_offset;
  return port_offset;
}

void sleep_for_boot() { std::this_thread::sleep_for(8s); }

/**
 * Test that a node that is full (connected to *max_connection* node)
 * don't accept others connections
 */
TEST(INTEGRATION, full_node) {
  Port port_offset = random_port();
  auto bot0 = std::make_unique<BotTest>("bot0.json", port_offset);
  bot0->set_max_incoming_connections(0);
  auto bot1 = std::make_unique<BotTest>("bot1.json", port_offset);
  std::this_thread::sleep_for(bot0->unreachable_timeout());

  ASSERT_EQ(bot0->peers().size(), 2) << bot0->peers();
  ASSERT_EQ(bot0->connected_peers().size(), 0);
  ASSERT_EQ(bot1->peers().size(), 2);
  ASSERT_EQ(bot1->connected_peers().size(), 0);

  const auto unavailable = static_cast<messages::Peer::Status>(
      messages::Peer::UNREACHABLE | messages::Peer::DISCONNECTED);
  ASSERT_TRUE(bot0->check_peer_status_by_port(1338, unavailable))
      << bot0->peers();
  ASSERT_TRUE(bot1->check_peer_status_by_port(1337, unavailable))
      << bot1->peers();
}

/**
 * Async test that a *connection ready* event is emited when node try to connect
 * Test that 3 node which know each other (through the config file) make a fully connected network
 */
TEST(INTEGRATION, simple_interaction) {
  int received_connection = 0;
  int received_deconnection = 0;
  Port port_offset = random_port();
  auto bot0 = std::make_unique<BotTest>("bot0.json", port_offset);
  bot0->subscribe(messages::Type::kConnectionReady,
                  [&](const messages::Header &header,
                      const messages::Body &body) { received_connection++; });

  auto bot1 = std::make_unique<BotTest>("bot1.json", port_offset);
  bot1->subscribe(messages::Type::kConnectionReady,
                  [&](const messages::Header &header,
                      const messages::Body &body) { received_connection++; });

  auto bot2 = std::make_unique<BotTest>("bot2.json", port_offset);
  bot2->subscribe(messages::Type::kConnectionReady,
                  [&](const messages::Header &header,
                      const messages::Body &body) { received_connection++; });

  sleep_for_boot();

  ASSERT_GT(received_connection, 0);
  ASSERT_EQ(received_deconnection, 0);

  std::this_thread::sleep_for(2s);

  ASSERT_TRUE(bot0->check_peers_ports({1338, 1339})) << bot0->peers();
  ASSERT_TRUE(bot1->check_peers_ports({1337, 1339})) << bot1->peers();
  ASSERT_TRUE(bot2->check_peers_ports({1337, 1338})) << bot2->peers();

  bot2.reset();
  std::this_thread::sleep_for(2s);
  bot1.reset();
  bot0.reset();
}

/**
 * Test that a node *A* which is already connected to another one *B*
 * mark the other one as disconnected when this one is killed
 * (*A* have 0 connected node when *B* is killed)
 */
TEST(INTEGRATION, disconnect) {
  Port port_offset = random_port();
  auto bot0 = std::make_unique<BotTest>("bot0.json", port_offset);
  auto bot1 = std::make_unique<BotTest>("bot1.json", port_offset);

  sleep_for_boot();

  ASSERT_EQ(bot0->connected_peers().size(), 1);
  ASSERT_EQ(bot1->connected_peers().size(), 1);

  bot1.reset();

  std::this_thread::sleep_for(1s);

  ASSERT_EQ(bot0->connected_peers().size(), 0);
}

/**
 * Async test that *disconnect* event is emited when nodes are killed
 * Test that the number or connected peer is correct after killing other node
 */
TEST(INTEGRATION, disconnect_message) {
  Port port_offset = random_port();
  bool message_received0 = false;
  bool message_received2 = false;
  auto bot0 = std::make_unique<BotTest>("bot0.json", port_offset);
  auto bot1 = std::make_unique<BotTest>("bot1.json", port_offset);
  auto bot2 = std::make_unique<BotTest>("bot2.json", port_offset);
  bot0->subscribe(
      messages::Type::kConnectionClosed,
      [&](const messages::Header &header, const messages::Body &body) {
        message_received0 = true;
      });
  bot2->subscribe(
      messages::Type::kConnectionClosed,
      [&](const messages::Header &header, const messages::Body &body) {
        message_received2 = true;
      });
  sleep_for_boot();

  ASSERT_EQ(bot0->connected_peers().size(), 2) << bot0->peers();
  ASSERT_EQ(bot1->connected_peers().size(), 2) << bot1->peers();
  ASSERT_EQ(bot2->connected_peers().size(), 2) << bot2->peers();

  bot1.reset();
  std::this_thread::sleep_for(2s);

  ASSERT_TRUE(message_received0) << bot0->peers();
  ASSERT_TRUE(message_received2) << bot2->peers();
  ASSERT_EQ(bot0->connected_peers().size(), 1) << bot0->peers();
  ASSERT_EQ(bot2->connected_peers().size(), 1) << bot2->peers();
}

/**
 * Test that when a node disconnect a bot it reconnect itself
 */
TEST(INTEGRATION, random_deconnection) {
  Port port_offset = random_port();
  auto bot0 = std::make_unique<BotTest>("bot0.json", port_offset);
  auto bot1 = std::make_unique<BotTest>("bot1.json", port_offset);
  auto bot2 = std::make_unique<BotTest>("bot2.json", port_offset);
  auto bot40 =
      std::make_unique<BotTest>("integration_propagation40.json", port_offset);

  ASSERT_TRUE(bot0->poll_connected_ports({1338, 1339, 13340})) << bot0->peers();
  ASSERT_TRUE(bot1->poll_connected_ports({1337, 1339, 13340})) << bot1->peers();
  ASSERT_TRUE(bot2->poll_connected_ports({1337, 1338, 13340})) << bot2->peers();
  ASSERT_TRUE(bot40->poll_connected_ports({1337, 1338, 1339})) << bot40->peers();

  auto random_peer = bot0->peers().begin();
  bot0->networking().terminate(random_peer->connection_id());

  ASSERT_TRUE(bot0->poll_connected_ports({1338, 1339, 13340})) << bot0->peers();
  ASSERT_TRUE(bot1->poll_connected_ports({1337, 1339, 13340})) << bot1->peers();
  ASSERT_TRUE(bot2->poll_connected_ports({1337, 1338, 13340})) << bot2->peers();
  ASSERT_TRUE(bot40->poll_connected_ports({1337, 1338, 1339})) << bot40->peers();
}

/**
 * Test that 2 nodes that don't know each other directly (through the config file)
 * can be connected with the help of a third node which is connected to them
 * (that is, a node can pass on it's peer list to others node)
 */
TEST(INTEGRATION, neighbors_propagation) {
  Port port_offset = random_port();
  auto bot0 =
      std::make_unique<BotTest>("integration_propagation0.json", port_offset);
  auto bot1 =
      std::make_unique<BotTest>("integration_propagation1.json", port_offset);
  auto bot2 =
      std::make_unique<BotTest>("integration_propagation2.json", port_offset);
  sleep_for_boot();

  ASSERT_TRUE(bot0->check_is_connected({1338, 1339})) << bot0->peers();
  ASSERT_TRUE(bot1->check_is_connected({1337})) << bot1->peers();
  ASSERT_TRUE(bot2->check_is_connected({1337})) << bot2->peers();

  // 2 sec for regular update (peer_list),
  // 2 sec for regular update (hello),
  // 7 sec for unreachable to disconnected,
  // 1 sec for some margin
  std::this_thread::sleep_for(16s);

  ASSERT_TRUE(bot0->check_peers_ports({1338, 1339})) << bot0->peers();
  ASSERT_TRUE(bot1->check_peers_ports({1337, 1339})) << bot1->peers();
  ASSERT_TRUE(bot2->check_peers_ports({1337, 1338})) << bot2->peers();
}

/**
 * Test that nodes that know each others make a fully connected network
 */
TEST(INTEGRATION, neighbors_connections) {
  Port port_offset = random_port();
  auto bot0 = std::make_unique<BotTest>("bot0.json", port_offset);
  auto bot1 = std::make_unique<BotTest>("bot1.json", port_offset);
  auto bot2 = std::make_unique<BotTest>("bot2.json", port_offset);

  sleep_for_boot();

  ASSERT_TRUE(bot0->check_peers_ports({1338, 1339})) << bot0->peers();
  ASSERT_TRUE(bot1->check_peers_ports({1337, 1339})) << bot1->peers();
  ASSERT_TRUE(bot2->check_peers_ports({1337, 1338})) << bot2->peers();
}

/**
 * Test that 2 node can connect even when only only know the other
 */
TEST(INTEGRATION, key_gen_connection) {
  Port port_offset = random_port();
  auto bot0 = std::make_unique<BotTest>("bot0.json", port_offset);
  auto bot50 =
      std::make_unique<BotTest>("integration_propagation50.json", port_offset);

  sleep_for_boot();

  ASSERT_TRUE(bot0->check_peers_ports({13350})) << bot0->peers();
  ASSERT_TRUE(bot50->check_peers_ports({1337})) << bot50->peers();
}

/**
 * Test that a node can connect to an already fully connected network
 */
TEST(INTEGRATION, fullfill_network) {
  // Add a fourth node to get a complete graph
  Port port_offset = random_port();
  auto bot0 = std::make_unique<BotTest>("bot0.json", port_offset);
  auto bot1 = std::make_unique<BotTest>("bot1.json", port_offset);
  auto bot2 = std::make_unique<BotTest>("bot2.json", port_offset);

  sleep_for_boot();

  // Now adding last node
  auto bot40 =
      std::make_unique<BotTest>("integration_propagation40.json", port_offset);

  sleep_for_boot();
  // TODO add a fifth node that shouldn't be able to connect (no *incoming_max_connection* slot available)
  ASSERT_TRUE(bot0->check_peers_ports({1338, 1339, 13340})) << bot0->peers();
  ASSERT_TRUE(bot1->check_peers_ports({1337, 1339, 13340})) << bot1->peers();
  ASSERT_TRUE(bot2->check_peers_ports({1337, 1338, 13340})) << bot2->peers();
  ASSERT_TRUE(bot40->check_peers_ports({1337, 1338, 1339})) << bot40->peers();
}

/**
 * Test that a node can't connect to a network without connection slot available
 * then can connect if one node of the network is killed
 * Async test that node receive *disconnect* event when a node is killed
 */
TEST(INTEGRATION, connection_opportunity) {
  int bot0_disconnect_received = 0;
  int bot1_disconnect_received = 0;
  int bot2_disconnect_received = 0;
  Port port_offset = random_port();
  auto bot0 = std::make_unique<BotTest>("bot0.json", port_offset);
  auto bot1 = std::make_unique<BotTest>("bot1.json", port_offset);
  auto bot2 = std::make_unique<BotTest>("bot2.json", port_offset);
  auto bot3 =
      std::make_unique<BotTest>("integration_propagation40.json", port_offset);
  bot0->subscribe(
      messages::Type::kConnectionClosed,
      [&](const messages::Header &header, const messages::Body &body) {
        auto remote_port = body.connection_closed().peer().port();
        if (remote_port) {
          bot0_disconnect_received++;
        }
      });
  bot1->subscribe(
      messages::Type::kConnectionClosed,
      [&](const messages::Header &header, const messages::Body &body) {
        auto remote_port = body.connection_closed().peer().port();
        if (remote_port) {
          bot1_disconnect_received++;
        }
      });
  bot2->subscribe(
      messages::Type::kConnectionClosed,
      [&](const messages::Header &header, const messages::Body &body) {
        auto remote_port = body.connection_closed().peer().port();
        if (remote_port) {
          bot2_disconnect_received++;
        }
      });
  sleep_for_boot();
  sleep_for_boot();
  LOG_INFO << "date";
  ASSERT_TRUE(bot0->check_peers_ports({1338, 1339, 13340}));
  ASSERT_TRUE(bot1->check_peers_ports({1337, 1339, 13340}));
  ASSERT_TRUE(bot2->check_peers_ports({1337, 1338, 13340}));
  ASSERT_TRUE(bot3->check_peers_ports({1337, 1338, 1339}));
  ASSERT_EQ(bot0_disconnect_received, 0);
  ASSERT_EQ(bot1_disconnect_received, 0);
  ASSERT_EQ(bot2_disconnect_received, 0);

  // Create additional node that cannot connect
  auto bot4 =
      std::make_unique<BotTest>("integration_propagation41.json", port_offset);
  std::this_thread::sleep_for(2s);

  ASSERT_TRUE(bot0->check_peers_ports({1338, 1339, 13340})) << bot0->peers();
  ASSERT_TRUE(bot1->check_peers_ports({1337, 1339, 13340})) << bot1->peers();
  ASSERT_TRUE(bot2->check_peers_ports({1337, 1338, 13340})) << bot2->peers();
  ASSERT_TRUE(bot3->check_peers_ports({1337, 1338, 1339})) << bot3->peers();

  // Check no change on other nodes
  ASSERT_TRUE(bot4->check_peers_ports({}));

  // Destroy a connected bot.
  bot3.reset();
  std::this_thread::sleep_for(15s);

  ASSERT_TRUE(bot0->check_peers_ports({1338, 1339, 13341})) << bot0->peers();
  ASSERT_TRUE(bot1->check_peers_ports({1337, 1339, 13341})) << bot1->peers();
  ASSERT_TRUE(bot2->check_peers_ports({1337, 1338, 13341})) << bot2->peers();
  ASSERT_TRUE(bot4->check_peers_ports({1337, 1338, 1339})) << bot4->peers();
}

/**
 * Test that a node can't connect to a network without connection slot available
 * then can connect if one node increase it's *max_connection* slot
 */
TEST(INTEGRATION, connection_opportunity_update) {
  // Check a node excluded from a connected graph can connect after a node of
  // the complete graph accept one more connection.
  Port port_offset = random_port();
  auto bot0 = std::make_unique<BotTest>("bot0.json", port_offset);
  auto bot1 = std::make_unique<BotTest>("bot1.json", port_offset);
  auto bot2 = std::make_unique<BotTest>("bot2.json", port_offset);
  auto bot3 =
      std::make_unique<BotTest>("integration_propagation40.json", port_offset);
  sleep_for_boot();
  std::this_thread::sleep_for(bot0->unreachable_timeout() + 2s);

  ASSERT_TRUE(bot0->check_peers_ports({1338, 1339, 13340})) << bot0->peers();
  ASSERT_TRUE(bot1->check_peers_ports({1337, 1339, 13340})) << bot1->peers();
  ASSERT_TRUE(bot2->check_peers_ports({1337, 1338, 13340})) << bot2->peers();
  ASSERT_TRUE(bot3->check_peers_ports({1337, 1338, 1339})) << bot3->peers();

  // Create additional node that cannot connect
  auto bot4 =
      std::make_unique<BotTest>("integration_propagation50.json", port_offset);
  std::this_thread::sleep_for(2s);

  // Check no change on other nodes
  ASSERT_TRUE(bot0->check_peers_ports({1338, 1339, 13340}));
  ASSERT_TRUE(bot1->check_peers_ports({1337, 1339, 13340}));
  ASSERT_TRUE(bot2->check_peers_ports({1337, 1338, 13340}));
  ASSERT_TRUE(bot3->check_peers_ports({1337, 1338, 1339}));
  ASSERT_TRUE(bot4->check_peers_ports({}));

  // Make bot3 accept one more connection
  bot3->set_max_incoming_connections(4);
  std::this_thread::sleep_for(bot3->unreachable_timeout() + 10s);

  ASSERT_TRUE(bot0->check_peers_ports({1338, 1339, 13340})) << bot0->peers();
  ASSERT_TRUE(bot1->check_peers_ports({1337, 1339, 13340})) << bot1->peers();
  ASSERT_TRUE(bot2->check_peers_ports({1337, 1338, 13340})) << bot2->peers();
  ASSERT_TRUE(bot3->check_peers_ports({1337, 1338, 1339, 13350}))
      << bot3->peers();
  ASSERT_TRUE(bot4->check_peers_ports({13340})) << bot4->peers();
}

/**
 * Test that a bot which is connected to a network can reconnect to a different network
 * if all it's peer are disconnected
 */
TEST(INTEGRATION, connection_reconfig) {
  // Test that new nodes can form a graph even if first node are all
  // disconnected.
  Port port_offset = random_port();
  auto bot0 = std::make_unique<BotTest>("bot0.json", port_offset);
  auto bot1 = std::make_unique<BotTest>("bot1.json", port_offset);
  auto bot2 = std::make_unique<BotTest>("bot2.json", port_offset);
  auto bot3 =
      std::make_unique<BotTest>("integration_propagation40.json", port_offset);
  sleep_for_boot();
  sleep_for_boot();

  ASSERT_TRUE(bot0->check_peers_ports({1338, 1339, 13340})) << bot0->peers();
  ASSERT_TRUE(bot1->check_peers_ports({1337, 1339, 13340})) << bot1->peers();
  ASSERT_TRUE(bot2->check_peers_ports({1337, 1338, 13340})) << bot2->peers();
  ASSERT_TRUE(bot3->check_peers_ports({1337, 1338, 1339})) << bot3->peers();

  // Now adding peers that know only one bot of the network.
  auto bot4 =
      std::make_unique<BotTest>("integration_propagation50.json", port_offset);
  auto bot5 =
      std::make_unique<BotTest>("integration_propagation51.json", port_offset);
  auto bot6 =
      std::make_unique<BotTest>("integration_propagation52.json", port_offset);

  // give them more time, they need to ask bot0 for more peer
  sleep_for_boot();
  sleep_for_boot();

  // Check there is no change for current network.
  ASSERT_TRUE(bot0->check_peers_ports({1338, 1339, 13340})) << bot0->peers();
  ASSERT_TRUE(bot1->check_peers_ports({1337, 1339, 13340})) << bot1->peers();
  ASSERT_TRUE(bot2->check_peers_ports({1337, 1338, 13340})) << bot2->peers();
  ASSERT_TRUE(bot3->check_peers_ports({1337, 1338, 1339})) << bot3->peers();

  sleep_for_boot();
  sleep_for_boot();
  ASSERT_TRUE(bot4->check_peers_ports({13351, 13352})) << bot4->peers();
  ASSERT_TRUE(bot5->check_peers_ports({13350, 13352})) << bot5->peers();
  ASSERT_TRUE(bot6->check_peers_ports({13350, 13351})) << bot6->peers();

  const auto unavailable = static_cast<messages::Peer::Status>(
      messages::Peer::UNREACHABLE | messages::Peer::DISCONNECTED);
  ASSERT_TRUE(bot0->check_peer_status_by_port(13350, unavailable))
      << bot0->peers();
  ASSERT_TRUE(bot0->check_peer_status_by_port(13351, unavailable))
      << bot0->peers();
  ASSERT_TRUE(bot0->check_peer_status_by_port(13352, unavailable))
      << bot0->peers();

  // Delete 3 first bots.
  bot1.reset();
  bot2.reset();
  bot3.reset();
  // worst case scenario, wait 10 sec after reset()
  std::this_thread::sleep_for(14s);

  ASSERT_TRUE(bot0->check_peers_ports({13350, 13351, 13352}));
  ASSERT_TRUE(bot4->check_peers_ports({1337, 13351, 13352}));
  ASSERT_TRUE(bot5->check_peers_ports({1337, 13350, 13352}));
  ASSERT_TRUE(bot6->check_peers_ports({1337, 13350, 13351}));
}

TEST(INTEGRATION, ignore_bad_message) {
  Port port_offset = random_port();
  auto bot0 = std::make_unique<BotTest>("bot0.json", port_offset);
  auto bot1 = std::make_unique<BotTest>("bot1.json", port_offset);
  bot1->subscribe(messages::Type::kGetPeers, [](const messages::Header &header,
                                                const messages::Body &body) {
    EXPECT_EQ(header.version(), neuro::MessageVersion);
  });

  sleep_for_boot();

  ASSERT_TRUE(bot0->check_peers_ports({1338}));
  ASSERT_TRUE(bot1->check_peers_ports({1337}));

  messages::Message msg;
  auto *header = msg.mutable_header();
  messages::fill_header(header);
  msg.add_bodies()->mutable_get_peers();
  header->set_version(neuro::MessageVersion + "100");
  bot0->send_all(msg);
}

TEST(INTEGRATION, keep_max_connections) {
  Port port_offset = random_port();
  auto bot0 = std::make_unique<BotTest>("bot0.json", port_offset);
  bot0->set_max_incoming_connections(1);
  auto bot1 = std::make_unique<BotTest>("bot1.json", port_offset);
  sleep_for_boot();
  auto bot2 = std::make_unique<BotTest>("bot2.json", port_offset);
  sleep_for_boot();

  ASSERT_TRUE(bot0->check_peers_ports({1338})) << bot0->peers();
  ASSERT_TRUE(bot1->check_peers_ports({1337, 1339})) << bot1->peers();
  ASSERT_TRUE(bot2->check_peers_ports({1338})) << bot2->peers();
}

TEST(INTEGRATION, handler_get_block) {
  Port port_offset = random_port();
  auto fake_bot = std::make_unique<BotTest>("bot0.json", port_offset);
  auto bot = std::make_unique<BotTest>("bot1.json", port_offset);
  sleep_for_boot();
  fake_bot->unsubscribe();

  auto get_block_id = 0;
  bot->subscribe(messages::Type::kGetBlock, [&](const messages::Header &header,
                                                const messages::Body &body) {
    get_block_id = header.id();
  });

  auto received_block = false;
  messages::Subscriber fake_bot_subscriber(&fake_bot->queue());
  fake_bot_subscriber.subscribe(
      messages::Type::kBlock,
      [&](const messages::Header &header, const messages::Body &body) {
        LOG_DEBUG << "header " << header;
        if (header.has_request_id() && header.request_id() == get_block_id) {
          LOG_DEBUG << "received block " << body.block();
          ASSERT_EQ(fake_bot->block0(), body.block());
          received_block = true;
        }
      });

  messages::Message message;
  messages::fill_header(message.mutable_header());
  auto get_block = message.add_bodies()->mutable_get_block();
  get_block->set_height(0);
  get_block->set_count(1);
  // ask for block 0
  fake_bot->send_all(message);
  std::this_thread::sleep_for(3s);
  ASSERT_TRUE(received_block);
}

}  // namespace neuro::tests
