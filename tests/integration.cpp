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

class BotTest {
 private:
  std::unique_ptr<neuro::Bot> _bot;
  Port _port_offset;

  void apply_port_offset(messages::config::Config &config) {
    auto port = config.networking().tcp().port();
    messages::config::Tcp *tcp_config =
        config.mutable_networking()->mutable_tcp();
    tcp_config->set_port(port + _port_offset);

    for (auto &peer : *tcp_config->mutable_peers()) {
      auto peer_port = peer.port();
      peer.set_port(peer_port + _port_offset);
    }
  }

 public:
  explicit BotTest(const std::string configPath, const Port port_offset = 0)
      : _port_offset(port_offset) {
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

  auto &queue() { return _bot->_queue; }

  const std::chrono::seconds unreachable_timeout() {
    auto t =_bot->_config.networking().default_next_update_time();
    return std::chrono::seconds{t};
  }

  const std::chrono::seconds connecting_timeout() {
    auto t = _bot->_config.networking().connecting_next_update_time();
    return std::chrono::seconds{t};
  }

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
      for (const auto &peer : _bot->connected_peers()) {
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

  int nb_blocks() { return _bot->_ledger->total_nb_blocks(); }

  void add_block() {
    messages::TaggedBlock new_block;
    neuro::tooling::blockgen::blockgen_from_last_db_block(
        new_block.mutable_block(), _bot->_ledger, 1, 0);
    _bot->_ledger->insert_block(new_block);
  }

  void unsubscribe() {
    _bot->_subscriber.unsubscribe();
  }

  messages::Block block0() {
    messages::Block block0;
    _bot->_ledger->get_block(0, &block0);
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

void sleep_for_boot() {
  std::this_thread::sleep_for(2s);
}

TEST(INTEGRATION, full_node) {
  // Try to connect to a bot that is full. The full node should me marked as
  // UNREACHABLE and the node that initiated the connection should be marked as
  // UNREACHABLE too
  Port port_offset = random_port();
  BotTest bot0("bot0.json", port_offset);
  bot0.set_max_incoming_connections(0);
  BotTest bot1("bot1.json", port_offset);
  std::this_thread::sleep_for(6s);

  ASSERT_EQ(bot0.peers().size(), 2) << bot0.peers();
  ASSERT_EQ(bot0->connected_peers().size(), 0);
  ASSERT_EQ(bot1.peers().size(), 2);
  ASSERT_EQ(bot1->connected_peers().size(), 0);

  const auto unavailable = static_cast<messages::Peer::Status>(
      messages::Peer::UNREACHABLE | messages::Peer::DISCONNECTED);
  ASSERT_TRUE(bot0.check_peer_status_by_port(1338, unavailable))
      << bot0->peers();
  ASSERT_TRUE(bot1.check_peer_status_by_port(1337, unavailable))
      << bot1->peers();
}

TEST(INTEGRATION, simple_interaction) {
  int received_connection = 0;
  int received_deconnection = 0;
  Port port_offset = random_port();
  BotTest bot0("bot0.json", port_offset);
  bot0->subscribe(messages::Type::kConnectionReady,
                  [&](const messages::Header &header,
                      const messages::Body &body) { received_connection++; });

  BotTest bot1("bot1.json", port_offset);
  bot1->subscribe(messages::Type::kConnectionReady,
                  [&](const messages::Header &header,
                      const messages::Body &body) { received_connection++; });

  BotTest bot2("bot2.json", port_offset);
  bot2->subscribe(messages::Type::kConnectionReady,
                  [&](const messages::Header &header,
                      const messages::Body &body) { received_connection++; });

  std::this_thread::sleep_for(2s);

  ASSERT_GT(received_connection, 0);
  ASSERT_EQ(received_deconnection, 0);

  std::this_thread::sleep_for(2s);

  ASSERT_TRUE(bot0.check_peers_ports({1338, 1339})) << bot0->peers();
  ASSERT_TRUE(bot1.check_peers_ports({1337, 1339})) << bot1->peers();
  ASSERT_TRUE(bot2.check_peers_ports({1337, 1338})) << bot2->peers();

  bot2.operator->().reset();
  std::this_thread::sleep_for(2s);
  bot1.operator->().reset();
  bot0.operator->().reset();
}

TEST(INTEGRATION, disconnect) {
  Port port_offset = random_port();
  BotTest bot0("bot0.json", port_offset);
  BotTest bot1("bot1.json", port_offset);

  std::this_thread::sleep_for(2s);

  ASSERT_EQ(bot0->connected_peers().size(), 1);
  ASSERT_EQ(bot1->connected_peers().size(), 1);

  bot1.operator->().reset();

  std::this_thread::sleep_for(2s);

  ASSERT_EQ(bot0->connected_peers().size(), 0);
}

TEST(INTEGRATION, disconnect_message) {
  Port port_offset = random_port();
  bool message_received0 = false;
  bool message_received2 = false;
  BotTest bot0("bot0.json", port_offset);
  BotTest bot1("bot1.json", port_offset);
  BotTest bot2("bot2.json", port_offset);
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

  ASSERT_EQ(bot0->connected_peers().size(), 2) << bot0.peers();
  ASSERT_EQ(bot1->connected_peers().size(), 2) << bot1.peers();
  ASSERT_EQ(bot2->connected_peers().size(), 2) << bot2.peers();

  bot1.operator->().reset();
  std::this_thread::sleep_for(2s);

  ASSERT_TRUE(message_received0) << bot0.peers();
  ASSERT_TRUE(message_received2) << bot2.peers();
  ASSERT_EQ(bot0->connected_peers().size(), 1) << bot0->peers();
  ASSERT_EQ(bot2->connected_peers().size(), 1) << bot1->peers();
}

TEST(INTEGRATION, neighbors_propagation) {
  Port port_offset = random_port();
  BotTest bot0("integration_propagation0.json", port_offset);
  BotTest bot1("integration_propagation1.json", port_offset);
  BotTest bot2("integration_propagation2.json", port_offset);
  std::this_thread::sleep_for(2s);

  ASSERT_TRUE(bot0.check_is_connected({1338, 1339})) << bot0->peers();
  ASSERT_TRUE(bot1.check_is_connected({1337})) << bot1->peers();
  ASSERT_TRUE(bot2.check_is_connected({1337})) << bot2->peers();

  // 2 sec for regular update (peer_list),
  // 2 sec for regular update (hello),
  // 7 sec for unreachable to disconnected,
  // 1 sec for some margin
  std::this_thread::sleep_for(12s);

  ASSERT_TRUE(bot0.check_peers_ports({1338, 1339})) << bot0->peers();
  ASSERT_TRUE(bot1.check_peers_ports({1337, 1339})) << bot1->peers();
  ASSERT_TRUE(bot2.check_peers_ports({1337, 1338})) << bot2->peers();
}

TEST(INTEGRATION, neighbors_connections) {
  Port port_offset = random_port();
  BotTest bot0("bot0.json", port_offset);
  BotTest bot1("bot1.json", port_offset);
  BotTest bot2("bot2.json", port_offset);

  std::this_thread::sleep_for(2s);

  ASSERT_TRUE(bot0.check_peers_ports({1338, 1339})) << bot0->peers();
  ASSERT_TRUE(bot1.check_peers_ports({1337, 1339})) << bot1->peers();
  ASSERT_TRUE(bot2.check_peers_ports({1337, 1338})) << bot2->peers();
}

TEST(INTEGRATION, key_gen_connection) {
  // Check an unknown bot with unknown signature can connect to bot0.
  Port port_offset = random_port();
  BotTest bot0("bot0.json", port_offset);
  BotTest bot50("integration_propagation50.json", port_offset);

  std::this_thread::sleep_for(2s);

  ASSERT_TRUE(bot0.check_peers_ports({13350})) << bot0->peers();
  ASSERT_TRUE(bot50.check_peers_ports({1337})) << bot50->peers();
}

TEST(INTEGRATION, fullfill_network) {
  // Add a fourth node to get a complete graph
  Port port_offset = random_port();
  BotTest bot0("bot0.json", port_offset);
  BotTest bot1("bot1.json", port_offset);
  BotTest bot2("bot2.json", port_offset);

  std::this_thread::sleep_for(2s);

  // Now adding last node
  BotTest bot40("integration_propagation40.json", port_offset);

  std::this_thread::sleep_for(4s);

  ASSERT_TRUE(bot0.check_peers_ports({1338, 1339, 13340})) << bot0->peers();
  ASSERT_TRUE(bot1.check_peers_ports({1337, 1339, 13340})) << bot1->peers();
  ASSERT_TRUE(bot2.check_peers_ports({1337, 1338, 13340})) << bot2->peers();
  ASSERT_TRUE(bot40.check_peers_ports({1337, 1338, 1339})) << bot40->peers();
}

TEST(INTEGRATION, connection_opportunity) {
  // Check a node excluded from a connected graph can connect after a node of
  // the complete graph is destroyed.
  int bot0_disconnect_received = 0;
  int bot1_disconnect_received = 0;
  int bot2_disconnect_received = 0;
  Port port_offset = random_port();
  BotTest bot0("bot0.json", port_offset);
  BotTest bot1("bot1.json", port_offset);
  BotTest bot2("bot2.json", port_offset);
  BotTest bot3("integration_propagation40.json", port_offset);
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
  std::this_thread::sleep_for(2s);
  LOG_INFO  << "date";
  ASSERT_TRUE(bot0.check_peers_ports({1338, 1339, 13340}));
  ASSERT_TRUE(bot1.check_peers_ports({1337, 1339, 13340}));
  ASSERT_TRUE(bot2.check_peers_ports({1337, 1338, 13340}));
  ASSERT_TRUE(bot3.check_peers_ports({1337, 1338, 1339}));
  ASSERT_EQ(bot0_disconnect_received, 0);
  ASSERT_EQ(bot1_disconnect_received, 0);
  ASSERT_EQ(bot2_disconnect_received, 0);

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
  Port port_offset = random_port();
  BotTest bot0("bot0.json", port_offset);
  BotTest bot1("bot1.json", port_offset);
  BotTest bot2("bot2.json", port_offset);
  BotTest bot3("integration_propagation40.json", port_offset);
  std::this_thread::sleep_for(5s);

  ASSERT_TRUE(bot0.check_peers_ports({1338, 1339, 13340})) << bot0.peers();
  ASSERT_TRUE(bot1.check_peers_ports({1337, 1339, 13340})) << bot1.peers();
  ASSERT_TRUE(bot2.check_peers_ports({1337, 1338, 13340})) << bot2.peers();
  ASSERT_TRUE(bot3.check_peers_ports({1337, 1338, 1339})) << bot3.peers();

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
  std::this_thread::sleep_for(10s);
  ASSERT_TRUE(bot0.check_peers_ports({1338, 1339, 13340})) << bot0.peers();
  ASSERT_TRUE(bot1.check_peers_ports({1337, 1339, 13340})) << bot1.peers();
  ASSERT_TRUE(bot2.check_peers_ports({1337, 1338, 13340})) << bot2.peers();
  ASSERT_TRUE(bot3.check_peers_ports({1337, 1338, 1339, 13350}))
      << bot3.peers();
  ASSERT_TRUE(bot4.check_peers_ports({13340})) << bot4.peers();
}

TEST(INTEGRATION, connection_reconfig) {
  // Test that new nodes can form a graph even if first node are all
  // disconnected.
  Port port_offset = random_port();
  BotTest bot0("bot0.json", port_offset);
  BotTest bot1("bot1.json", port_offset);
  BotTest bot2("bot2.json", port_offset);
  BotTest bot3("integration_propagation40.json", port_offset);
  std::this_thread::sleep_for(2s);

  ASSERT_TRUE(bot0.check_peers_ports({1338, 1339, 13340})) << bot0->peers();
  ASSERT_TRUE(bot1.check_peers_ports({1337, 1339, 13340})) << bot1->peers();
  ASSERT_TRUE(bot2.check_peers_ports({1337, 1338, 13340})) << bot2->peers();
  ASSERT_TRUE(bot3.check_peers_ports({1337, 1338, 1339})) << bot3->peers();

  // Now adding peers that know only one bot of the network.
  BotTest bot4("integration_propagation50.json", port_offset);
  BotTest bot5("integration_propagation51.json", port_offset);
  BotTest bot6("integration_propagation52.json", port_offset);
  // give them more time, they need to ask bot0 for more peer
  std::this_thread::sleep_for(4s);


  auto print_peers = [&bot0, &bot1, &bot2, &bot3, &bot4, &bot5, &bot6]() -> std::string {
    std::ostringstream os;
    os << "all peers" << std::endl
    << "bot0" << std::endl << bot0->peers() << std::endl
    << "bot1" << std::endl << bot1->peers() << std::endl
    << "bot2" << std::endl << bot2->peers() << std::endl
    << "bot3" << std::endl << bot3->peers() << std::endl
    << "bot4" << std::endl << bot4->peers() << std::endl
    << "bot5" << std::endl << bot5->peers() << std::endl
    << "bot6" << std::endl << bot6->peers() << std::endl
    << "==================================================" << std::endl;
    return os.str();
  };
  // Check there is no change for current network.
  ASSERT_TRUE(bot0.check_peers_ports({1338, 1339, 13340})) << print_peers();
  ASSERT_TRUE(bot1.check_peers_ports({1337, 1339, 13340})) << print_peers();
  ASSERT_TRUE(bot2.check_peers_ports({1337, 1338, 13340})) << print_peers();
  ASSERT_TRUE(bot3.check_peers_ports({1337, 1338, 1339}))  << print_peers();
  ASSERT_TRUE(bot4.check_peers_ports({13351, 13352}))      << print_peers();
  ASSERT_TRUE(bot5.check_peers_ports({13350, 13352}))      << print_peers();
  ASSERT_TRUE(bot6.check_peers_ports({13350, 13351}))      << print_peers();

  const auto unavailable = static_cast<messages::Peer::Status>(
      messages::Peer::UNREACHABLE | messages::Peer::DISCONNECTED);
  ASSERT_TRUE(bot0.check_peer_status_by_port(13350, unavailable))
      << bot0->peers();
  ASSERT_TRUE(bot0.check_peer_status_by_port(13351, unavailable))
      << bot0->peers();
  ASSERT_TRUE(bot0.check_peer_status_by_port(13352, unavailable))
      << bot0->peers();

  // Delete 3 first bots.
  bot1.operator->().reset();
  bot2.operator->().reset();
  bot3.operator->().reset();
  // worst case scenario, wait 8 sec after reset()
  std::this_thread::sleep_for(8s);

  ASSERT_TRUE(bot0.check_peers_ports({13350, 13351, 13352})) << print_peers();
  ASSERT_TRUE(bot4.check_peers_ports({1337, 13351, 13352}))  << print_peers();
  ASSERT_TRUE(bot5.check_peers_ports({1337, 13350, 13352}))  << print_peers();
  ASSERT_TRUE(bot6.check_peers_ports({1337, 13350, 13351}))  << print_peers();
}

TEST(INTEGRATION, ignore_bad_message) {
  Port port_offset = random_port();
  BotTest bot0("bot0.json", port_offset);
  BotTest bot1("bot1.json", port_offset);
  bot1->subscribe(messages::Type::kGetPeers, [](const messages::Header &header,
                                                const messages::Body &body) {
    EXPECT_EQ(header.version(), neuro::MessageVersion);
  });

  std::this_thread::sleep_for(2s);

  ASSERT_TRUE(bot0.check_peers_ports({1338}));
  ASSERT_TRUE(bot1.check_peers_ports({1337}));

  auto msg = std::make_shared<messages::Message>();
  auto *header = msg->mutable_header();
  messages::fill_header(header);
  msg->add_bodies()->mutable_get_peers();
  header->set_version(neuro::MessageVersion + 100);
  bot0.networking().send(msg);
}

TEST(INTEGRATION, keep_max_connections) {
  Port port_offset = random_port();
  BotTest bot0("bot0.json", port_offset);
  bot0.set_max_incoming_connections(1);
  std::this_thread::sleep_for(2s);
  BotTest bot1("bot1.json", port_offset);
  std::this_thread::sleep_for(4s);
  BotTest bot2("bot2.json", port_offset);
  std::this_thread::sleep_for(4s);

  ASSERT_TRUE(bot0.check_peers_ports({1338}));
  ASSERT_TRUE(bot1.check_peers_ports({1337, 1339}));
  ASSERT_TRUE(bot2.check_peers_ports({1338}));
}

TEST(INTEGRATION, handler_get_block) {
  Port port_offset = random_port();
  BotTest fake_bot("bot0.json", port_offset);
  fake_bot.unsubscribe();
  BotTest bot("bot1.json", port_offset);
  sleep_for_boot();

  auto get_block_id = 0;
  bot->subscribe(messages::Type::kGetBlock, [&](const messages::Header &header,
                                                 const messages::Body &body) {
    get_block_id = header.id();
  });

  auto received_block = false;
  messages::Subscriber fake_bot_subscriber(&fake_bot.queue());
  fake_bot_subscriber.subscribe(messages::Type::kBlock, [&](const messages::Header &header,
                                              const messages::Body &body) {
    if (header.request_id() == get_block_id) {
      ASSERT_EQ(fake_bot.block0(), body.block());
      received_block = true;
    }
  });


  auto message = std::make_shared<messages::Message>();
  messages::fill_header(message->mutable_header());
  auto get_block = message->add_bodies()->mutable_get_block();
  get_block->set_height(0);
  get_block->set_count(1);
  // ask for block 0
  fake_bot.networking().send(message);
  std::this_thread::sleep_for(3s);
  ASSERT_TRUE(received_block);
}

}  // namespace neuro::tests
