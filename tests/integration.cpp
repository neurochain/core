#include <gtest/gtest.h>
#include <chrono>
#include <sstream>
#include <thread>
#include "Bot.hpp"
#include "common/logger.hpp"
#include "common/types.hpp"
#include "consensus/PiiConsensus.hpp"
#include "crypto/Sign.hpp"
#include "ledger/LedgerMongodb.hpp"
#include "messages/Subscriber.hpp"
#include "messages/config/Config.hpp"
#include "tooling/blockgen.hpp"

namespace neuro {

namespace tests {

using namespace std::chrono_literals;

template <class C>
std::vector<typename C::value_type> vectorize(const C& container) {
  std::vector<typename C::value_type> ans(container.begin(), container.end());
  return ans;
}

class Listener {
 private:
  std::size_t _received_connection{0};
  std::size_t _received_deconnection{0};

 public:
  Listener() {}
  void handler_connection(const messages::Header &header,
                          const messages::Body &body) {
    LOG_DEBUG << __FILE__ << ": It entered the handler_connection !!";
    ++_received_connection;
  }
  void handler_deconnection(const messages::Header &header,
                            const messages::Body &body) {
    ++_received_deconnection;
  }

  std::size_t received_connection() const { return _received_connection; };
  std::size_t received_deconnection() const { return _received_deconnection; };
};

class BotTest {
 private:
  neuro::Bot _bot;

 public:
  BotTest(const messages::config::Config &conf) : _bot(conf) {}

  int nb_blocks() { return _bot._ledger->total_nb_blocks(); }

  void add_block() {
    messages::TaggedBlock new_block;
    neuro::tooling::blockgen::blockgen_from_last_db_block(new_block.mutable_block(),
                                                          _bot._ledger, 1, 0);
    _bot._ledger->insert_block(&new_block);
  }
};

TEST(INTEGRATION, simple_interaction) {
  Listener listener;
  Path config_path0("bot0.json");
  messages::config::Config config0(config_path0);
  auto bot0 = std::make_shared<Bot>(config0);
  messages::Subscriber subscriber0(bot0->queue());
  subscriber0.subscribe(
      messages::Type::kConnectionReady,
      [&listener](const messages::Header &header, const messages::Body &body) {
        listener.handler_connection(header, body);
      });

  Path config_path1("bot1.json");
  messages::config::Config config1(config_path1);
  auto bot1 = std::make_shared<Bot>(config1);
  messages::Subscriber subscriber1(bot1->queue());
  subscriber1.subscribe(
      messages::Type::kConnectionReady,
      [&listener](const messages::Header &header, const messages::Body &body) {
        listener.handler_connection(header, body);
      });

  Path config_path2("bot2.json");
  messages::config::Config config2(config_path2);
  auto bot2 = std::make_shared<Bot>(config2);
  messages::Subscriber subscriber2(bot2->queue());
  subscriber2.subscribe(
      messages::Type::kConnectionReady,
      [&listener](const messages::Header &header, const messages::Body &body) {
        listener.handler_connection(header, body);
      });

  std::this_thread::sleep_for(5s);

  LOG_DEBUG << "listener.received_connection() = "
            << listener.received_connection();

  LOG_DEBUG << "listener.received_deconnection() = "
            << listener.received_deconnection();

  ASSERT_GT(listener.received_connection(), 0);
  ASSERT_EQ(listener.received_deconnection(), 0);

  auto peers_bot0 = (bot0->connected_peers());
  auto peers_bot1 = (bot1->connected_peers());
  auto peers_bot2 = (bot2->connected_peers());

  ASSERT_EQ(peers_bot0.size(), peers_bot1.size());
  ASSERT_EQ(peers_bot1.size(), peers_bot2.size());
  ASSERT_EQ(peers_bot2.size(), 2);

  ASSERT_EQ(peers_bot0[0].endpoint(), "127.0.0.1");
  ASSERT_EQ(peers_bot0[1].endpoint(), "127.0.0.1");
  ASSERT_TRUE(peers_bot0[0].port() == 1338 || peers_bot0[0].port() == 1339);
  ASSERT_TRUE(peers_bot0[1].port() == 1338 || peers_bot0[1].port() == 1339);
  ASSERT_NE(peers_bot0[0].port(), peers_bot0[1].port());

  ASSERT_EQ(peers_bot1[0].endpoint(), "127.0.0.1");
  ASSERT_EQ(peers_bot1[1].endpoint(), "127.0.0.1");
  ASSERT_TRUE(peers_bot1[0].port() == 1337 || peers_bot1[0].port() == 1339);
  ASSERT_TRUE(peers_bot1[1].port() == 1337 || peers_bot1[1].port() == 1339);
  ASSERT_NE(peers_bot1[0].port(), peers_bot1[1].port());

  ASSERT_EQ(peers_bot2[0].endpoint(), "127.0.0.1");
  ASSERT_EQ(peers_bot2[1].endpoint(), "127.0.0.1");
  ASSERT_TRUE(peers_bot2[0].port() == 1337 || peers_bot2[0].port() == 1338);
  ASSERT_TRUE(peers_bot2[1].port() == 1337 || peers_bot2[1].port() == 1338);
  ASSERT_NE(peers_bot2[0].port(), peers_bot2[1].port());
  bot2.reset();
  bot1.reset();
  bot0.reset();
}

TEST(INTEGRATION, neighbors_propagation) {
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

  ASSERT_EQ(peers_bot0[0].endpoint(), "127.0.0.1");
  ASSERT_EQ(peers_bot0[1].endpoint(), "127.0.0.1");
  ASSERT_TRUE(peers_bot0[0].port() == 1338 || peers_bot0[0].port() == 1339);
  ASSERT_TRUE(peers_bot0[1].port() == 1338 || peers_bot0[1].port() == 1339);
  ASSERT_NE(peers_bot0[0].port(), peers_bot0[1].port());

  ASSERT_EQ(peers_bot1[0].endpoint(), "127.0.0.1");
  ASSERT_EQ(peers_bot1[1].endpoint(), "127.0.0.1");
  ASSERT_TRUE(peers_bot1[0].port() == 1337 || peers_bot1[0].port() == 1339);
  ASSERT_TRUE(peers_bot1[1].port() == 1337 || peers_bot1[1].port() == 1339);
  ASSERT_NE(peers_bot1[0].port(), peers_bot1[1].port());

  ASSERT_EQ(peers_bot2[0].endpoint(), "127.0.0.1");
  ASSERT_EQ(peers_bot2[1].endpoint(), "127.0.0.1");
  ASSERT_TRUE(peers_bot2[0].port() == 1337 || peers_bot2[0].port() == 1338);
  ASSERT_TRUE(peers_bot2[1].port() == 1337 || peers_bot2[1].port() == 1338);
  ASSERT_NE(peers_bot2[0].port(), peers_bot2[1].port());
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

  ASSERT_EQ(peers_bot0[0].endpoint(), "127.0.0.1");
  ASSERT_EQ(peers_bot0[1].endpoint(), "127.0.0.1");
  ASSERT_TRUE(peers_bot0[0].port() == 1338 || peers_bot0[0].port() == 1339);
  ASSERT_TRUE(peers_bot0[1].port() == 1338 || peers_bot0[1].port() == 1339);
  ASSERT_NE(peers_bot0[0].port(), peers_bot0[1].port());

  ASSERT_EQ(peers_bot1[0].endpoint(), "127.0.0.1");
  ASSERT_EQ(peers_bot1[1].endpoint(), "127.0.0.1");
  ASSERT_TRUE(peers_bot1[0].port() == 1337 || peers_bot1[0].port() == 1339);
  ASSERT_TRUE(peers_bot1[1].port() == 1337 || peers_bot1[1].port() == 1339);
  ASSERT_NE(peers_bot1[0].port(), peers_bot1[1].port());

  ASSERT_EQ(peers_bot2[0].endpoint(), "127.0.0.1");
  ASSERT_EQ(peers_bot2[1].endpoint(), "127.0.0.1");
  ASSERT_TRUE(peers_bot2[0].port() == 1337 || peers_bot2[0].port() == 1338);
  ASSERT_TRUE(peers_bot2[1].port() == 1337 || peers_bot2[1].port() == 1338);
  ASSERT_NE(peers_bot2[0].port(), peers_bot2[1].port());
}

TEST(INTEGRATION, neighbors_update) {
  Path config_path0("integration_update0.json");
  messages::config::Config config0(config_path0);
  auto bot0 = std::make_shared<Bot>(config0);
  std::this_thread::sleep_for(5s);
  Path config_path1("integration_update1.json");
  messages::config::Config config1(config_path1);
  auto bot1 = std::make_shared<Bot>(config1);
  std::this_thread::sleep_for(5s);
  Path config_path2("integration_update2.json");
  messages::config::Config config2(config_path2);
  auto bot2 = std::make_shared<Bot>(config2);

  std::this_thread::sleep_for(5s);

  auto peers_bot0 = vectorize(bot0->connected_peers());
  auto peers_bot1 = vectorize(bot1->connected_peers());
  auto peers_bot2 = vectorize(bot2->connected_peers());

  ASSERT_EQ(peers_bot0.size(), peers_bot1.size());
  ASSERT_EQ(peers_bot1.size(), peers_bot2.size());
  ASSERT_EQ(peers_bot2.size(), 2);

  ASSERT_EQ(peers_bot0[0].endpoint(), "127.0.0.1");
  ASSERT_EQ(peers_bot0[1].endpoint(), "127.0.0.1");
  ASSERT_TRUE(peers_bot0[0].port() == 1338 || peers_bot0[0].port() == 1339);
  ASSERT_TRUE(peers_bot0[1].port() == 1338 || peers_bot0[1].port() == 1339);
  ASSERT_NE(peers_bot0[0].port(), peers_bot0[1].port());

  ASSERT_EQ(peers_bot1[0].endpoint(), "127.0.0.1");
  ASSERT_EQ(peers_bot1[1].endpoint(), "127.0.0.1");
  ASSERT_TRUE(peers_bot1[0].port() == 1337 || peers_bot1[0].port() == 1339);
  ASSERT_TRUE(peers_bot1[1].port() == 1337 || peers_bot1[1].port() == 1339);
  ASSERT_NE(peers_bot1[0].port(), peers_bot1[1].port());

  ASSERT_EQ(peers_bot2[0].endpoint(), "127.0.0.1");
  ASSERT_EQ(peers_bot2[1].endpoint(), "127.0.0.1");
  ASSERT_TRUE(peers_bot2[0].port() == 1337 || peers_bot2[0].port() == 1338);
  ASSERT_TRUE(peers_bot2[1].port() == 1337 || peers_bot2[1].port() == 1338);
  ASSERT_NE(peers_bot2[0].port(), peers_bot2[1].port());
}

TEST(INTEGRATION, key_gen_connection) {
  // Check an unknown bot with unknown signature can connect to bot0.
  Path config_path0("bot0.json");
  messages::config::Config config0(config_path0);
  auto bot0 = std::make_shared<Bot>(config0);

  Path config_path1("bot_outsider.50-37.json");
  messages::config::Config config1(config_path1);
  auto bot1 = std::make_shared<Bot>(config1);

  std::this_thread::sleep_for(5s);

  auto peers_bot0 = vectorize(bot0->connected_peers());
  auto peers_bot1 = vectorize(bot1->connected_peers());

  ASSERT_EQ(peers_bot0.size(), 1);
  ASSERT_EQ(peers_bot1.size(), 1);

  ASSERT_EQ(peers_bot0[0].endpoint(), "127.0.0.1");
  ASSERT_TRUE(peers_bot0[0].port() == 1350);

  ASSERT_EQ(peers_bot1[0].endpoint(), "127.0.0.1");
  ASSERT_TRUE(peers_bot1[0].port() == 1337);
}

TEST(INTEGRATION, key_gen_connection_reverse) {
  // Check an unknown bot, set up first, with unknown signature can connect to
  // bot0 when bot0 is set up.
  Path config_path1("bot_outsider.50-37.json");
  messages::config::Config config1(config_path1);
  auto bot1 = std::make_shared<Bot>(config1);

  Path config_path0("bot0.json");
  messages::config::Config config0(config_path0);
  auto bot0 = std::make_shared<Bot>(config0);

  std::this_thread::sleep_for(5s);

  auto peers_bot0 = vectorize(bot0->connected_peers());
  auto peers_bot1 = vectorize(bot1->connected_peers());

  ASSERT_EQ(peers_bot0.size(), 1);
  ASSERT_EQ(peers_bot1.size(), 1);

  ASSERT_EQ(peers_bot0[0].endpoint(), "127.0.0.1");
  ASSERT_TRUE(peers_bot0[0].port() == 1350);

  ASSERT_EQ(peers_bot1[0].endpoint(), "127.0.0.1");
  ASSERT_TRUE(peers_bot1[0].port() == 1337);
}

TEST(INTEGRATION, fullfill_network) {
  // Add a fourth node to get a compleete graph
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
  Path config_path3("bot_outsider.40-38.39.json");
  messages::config::Config config3(config_path3);
  auto bot3 = std::make_shared<Bot>(config3);

  std::this_thread::sleep_for(5s);

  auto peers_bot0 = vectorize(bot0->connected_peers());
  auto peers_bot1 = vectorize(bot1->connected_peers());
  auto peers_bot2 = vectorize(bot2->connected_peers());
  auto peers_bot3 = vectorize(bot3->connected_peers());

  ASSERT_EQ(peers_bot0.size(), peers_bot1.size());
  ASSERT_EQ(peers_bot1.size(), peers_bot2.size());
  ASSERT_EQ(peers_bot2.size(), peers_bot3.size());
  ASSERT_EQ(peers_bot3.size(), 3);

  ASSERT_EQ(peers_bot0[0].endpoint(), "127.0.0.1");
  ASSERT_EQ(peers_bot0[1].endpoint(), "127.0.0.1");
  ASSERT_EQ(peers_bot0[2].endpoint(), "127.0.0.1");
  ASSERT_TRUE(peers_bot0[0].port() == 1338 || peers_bot0[0].port() == 1339 ||
              peers_bot0[0].port() == 1340);
  ASSERT_TRUE(peers_bot0[1].port() == 1338 || peers_bot0[1].port() == 1339 ||
              peers_bot0[1].port() == 1340);
  ASSERT_TRUE(peers_bot0[2].port() == 1338 || peers_bot0[2].port() == 1339 ||
              peers_bot0[2].port() == 1340);
  ASSERT_NE(peers_bot0[0].port(), peers_bot0[1].port());
  ASSERT_NE(peers_bot0[1].port(), peers_bot0[2].port());
  ASSERT_NE(peers_bot0[0].port(), peers_bot0[2].port());

  ASSERT_EQ(peers_bot1[0].endpoint(), "127.0.0.1");
  ASSERT_EQ(peers_bot1[1].endpoint(), "127.0.0.1");
  ASSERT_EQ(peers_bot1[2].endpoint(), "127.0.0.1");
  ASSERT_TRUE(peers_bot1[0].port() == 1337 || peers_bot1[0].port() == 1339 ||
              peers_bot1[0].port() == 1340);
  ASSERT_TRUE(peers_bot1[1].port() == 1337 || peers_bot1[1].port() == 1339 ||
              peers_bot1[1].port() == 1340);
  ASSERT_TRUE(peers_bot1[2].port() == 1337 || peers_bot1[2].port() == 1339 ||
              peers_bot1[2].port() == 1340);
  ASSERT_NE(peers_bot1[0].port(), peers_bot1[1].port());
  ASSERT_NE(peers_bot1[1].port(), peers_bot1[2].port());
  ASSERT_NE(peers_bot1[0].port(), peers_bot1[2].port());

  ASSERT_EQ(peers_bot2[0].endpoint(), "127.0.0.1");
  ASSERT_EQ(peers_bot2[1].endpoint(), "127.0.0.1");
  ASSERT_EQ(peers_bot2[2].endpoint(), "127.0.0.1");
  ASSERT_TRUE(peers_bot2[0].port() == 1337 || peers_bot2[0].port() == 1338 ||
              peers_bot2[0].port() == 1340);
  ASSERT_TRUE(peers_bot2[1].port() == 1337 || peers_bot2[1].port() == 1338 ||
              peers_bot2[1].port() == 1340);
  ASSERT_TRUE(peers_bot2[2].port() == 1337 || peers_bot2[2].port() == 1338 ||
              peers_bot2[2].port() == 1340);
  ASSERT_NE(peers_bot2[0].port(), peers_bot2[1].port());
  ASSERT_NE(peers_bot2[1].port(), peers_bot2[2].port());
  ASSERT_NE(peers_bot2[0].port(), peers_bot2[2].port());

  ASSERT_EQ(peers_bot3[0].endpoint(), "127.0.0.1");
  ASSERT_EQ(peers_bot3[1].endpoint(), "127.0.0.1");
  ASSERT_EQ(peers_bot3[2].endpoint(), "127.0.0.1");
  ASSERT_TRUE(peers_bot3[0].port() == 1337 || peers_bot3[0].port() == 1338 ||
              peers_bot3[0].port() == 1339);
  ASSERT_TRUE(peers_bot3[1].port() == 1337 || peers_bot3[1].port() == 1338 ||
              peers_bot3[1].port() == 1339);
  ASSERT_TRUE(peers_bot3[2].port() == 1337 || peers_bot3[2].port() == 1338 ||
              peers_bot3[2].port() == 1339);
  ASSERT_NE(peers_bot3[0].port(), peers_bot3[1].port());
  ASSERT_NE(peers_bot3[1].port(), peers_bot3[2].port());
  ASSERT_NE(peers_bot3[0].port(), peers_bot3[2].port());
}

TEST(INTEGRATION, connection_opportunity) {
  // Check a node excluded from a connected graph can connect after a node of the
  // compleete graph is destroyed.
  Path config_path0("bot0.json");
  messages::config::Config config0(config_path0);
  auto bot0 = std::make_shared<Bot>(config0);
  Path config_path1("bot1.json");
  messages::config::Config config1(config_path1);
  auto bot1 = std::make_shared<Bot>(config1);
  Path config_path2("bot2.json");
  messages::config::Config config2(config_path2);
  auto bot2 = std::make_shared<Bot>(config2);
  Path config_path3("bot_outsider.40-38.39.json");
  messages::config::Config config3(config_path3);
  auto bot3 = std::make_shared<Bot>(config3);

  std::this_thread::sleep_for(5s);

  auto peers_bot0 = vectorize(bot0->connected_peers());
  auto peers_bot1 = vectorize(bot1->connected_peers());
  auto peers_bot2 = vectorize(bot2->connected_peers());
  auto peers_bot3 = vectorize(bot3->connected_peers());

  ASSERT_EQ(peers_bot0.size(), peers_bot1.size());
  ASSERT_EQ(peers_bot1.size(), peers_bot2.size());
  ASSERT_EQ(peers_bot2.size(), peers_bot3.size());
  ASSERT_EQ(peers_bot3.size(), 3);

  ASSERT_EQ(peers_bot0[0].endpoint(), "127.0.0.1");
  ASSERT_EQ(peers_bot0[1].endpoint(), "127.0.0.1");
  ASSERT_EQ(peers_bot0[2].endpoint(), "127.0.0.1");
  ASSERT_TRUE(peers_bot0[0].port() == 1338 || peers_bot0[0].port() == 1339 ||
              peers_bot0[0].port() == 1340);
  ASSERT_TRUE(peers_bot0[1].port() == 1338 || peers_bot0[1].port() == 1339 ||
              peers_bot0[1].port() == 1340);
  ASSERT_TRUE(peers_bot0[2].port() == 1338 || peers_bot0[2].port() == 1339 ||
              peers_bot0[2].port() == 1340);
  ASSERT_NE(peers_bot0[0].port(), peers_bot0[1].port());
  ASSERT_NE(peers_bot0[1].port(), peers_bot0[2].port());
  ASSERT_NE(peers_bot0[0].port(), peers_bot0[2].port());

  ASSERT_EQ(peers_bot1[0].endpoint(), "127.0.0.1");
  ASSERT_EQ(peers_bot1[1].endpoint(), "127.0.0.1");
  ASSERT_EQ(peers_bot1[2].endpoint(), "127.0.0.1");
  ASSERT_TRUE(peers_bot1[0].port() == 1337 || peers_bot1[0].port() == 1339 ||
              peers_bot1[0].port() == 1340);
  ASSERT_TRUE(peers_bot1[1].port() == 1337 || peers_bot1[1].port() == 1339 ||
              peers_bot1[1].port() == 1340);
  ASSERT_TRUE(peers_bot1[2].port() == 1337 || peers_bot1[2].port() == 1339 ||
              peers_bot1[2].port() == 1340);
  ASSERT_NE(peers_bot1[0].port(), peers_bot1[1].port());
  ASSERT_NE(peers_bot1[1].port(), peers_bot1[2].port());
  ASSERT_NE(peers_bot1[0].port(), peers_bot1[2].port());

  ASSERT_EQ(peers_bot2[0].endpoint(), "127.0.0.1");
  ASSERT_EQ(peers_bot2[1].endpoint(), "127.0.0.1");
  ASSERT_EQ(peers_bot2[2].endpoint(), "127.0.0.1");
  ASSERT_TRUE(peers_bot2[0].port() == 1337 || peers_bot2[0].port() == 1338 ||
              peers_bot2[0].port() == 1340);
  ASSERT_TRUE(peers_bot2[1].port() == 1337 || peers_bot2[1].port() == 1338 ||
              peers_bot2[1].port() == 1340);
  ASSERT_TRUE(peers_bot2[2].port() == 1337 || peers_bot2[2].port() == 1338 ||
              peers_bot2[2].port() == 1340);
  ASSERT_NE(peers_bot2[0].port(), peers_bot2[1].port());
  ASSERT_NE(peers_bot2[1].port(), peers_bot2[2].port());
  ASSERT_NE(peers_bot2[0].port(), peers_bot2[2].port());

  ASSERT_EQ(peers_bot3[0].endpoint(), "127.0.0.1");
  ASSERT_EQ(peers_bot3[1].endpoint(), "127.0.0.1");
  ASSERT_EQ(peers_bot3[2].endpoint(), "127.0.0.1");
  ASSERT_TRUE(peers_bot3[0].port() == 1337 || peers_bot3[0].port() == 1338 ||
              peers_bot3[0].port() == 1339);
  ASSERT_TRUE(peers_bot3[1].port() == 1337 || peers_bot3[1].port() == 1338 ||
              peers_bot3[1].port() == 1339);
  ASSERT_TRUE(peers_bot3[2].port() == 1337 || peers_bot3[2].port() == 1338 ||
              peers_bot3[2].port() == 1339);
  ASSERT_NE(peers_bot3[0].port(), peers_bot3[1].port());
  ASSERT_NE(peers_bot3[1].port(), peers_bot3[2].port());
  ASSERT_NE(peers_bot3[0].port(), peers_bot3[2].port());

  // Create additional node that cannot connect
  Path config_path4("bot_outsider.41-38.39.json");
  messages::config::Config config4(config_path4);
  auto bot4 = std::make_shared<Bot>(config4);

  std::this_thread::sleep_for(5s);

  // Check no change on other nodes
  peers_bot0 = vectorize(bot0->connected_peers());
  peers_bot1 = vectorize(bot1->connected_peers());
  peers_bot2 = vectorize(bot2->connected_peers());
  peers_bot3 = vectorize(bot3->connected_peers());

  ASSERT_EQ(peers_bot0.size(), peers_bot1.size());
  ASSERT_EQ(peers_bot1.size(), peers_bot2.size());
  ASSERT_EQ(peers_bot2.size(), peers_bot3.size());
  ASSERT_EQ(peers_bot3.size(), 3);

  ASSERT_EQ(peers_bot0[0].endpoint(), "127.0.0.1");
  ASSERT_EQ(peers_bot0[1].endpoint(), "127.0.0.1");
  ASSERT_EQ(peers_bot0[2].endpoint(), "127.0.0.1");
  ASSERT_TRUE(peers_bot0[0].port() == 1338 || peers_bot0[0].port() == 1339 ||
              peers_bot0[0].port() == 1340);
  ASSERT_TRUE(peers_bot0[1].port() == 1338 || peers_bot0[1].port() == 1339 ||
              peers_bot0[1].port() == 1340);
  ASSERT_TRUE(peers_bot0[2].port() == 1338 || peers_bot0[2].port() == 1339 ||
              peers_bot0[2].port() == 1340);
  ASSERT_NE(peers_bot0[0].port(), peers_bot0[1].port());
  ASSERT_NE(peers_bot0[1].port(), peers_bot0[2].port());
  ASSERT_NE(peers_bot0[0].port(), peers_bot0[2].port());

  ASSERT_EQ(peers_bot1[0].endpoint(), "127.0.0.1");
  ASSERT_EQ(peers_bot1[1].endpoint(), "127.0.0.1");
  ASSERT_EQ(peers_bot1[2].endpoint(), "127.0.0.1");
  ASSERT_TRUE(peers_bot1[0].port() == 1337 || peers_bot1[0].port() == 1339 ||
              peers_bot1[0].port() == 1340);
  ASSERT_TRUE(peers_bot1[1].port() == 1337 || peers_bot1[1].port() == 1339 ||
              peers_bot1[1].port() == 1340);
  ASSERT_TRUE(peers_bot1[2].port() == 1337 || peers_bot1[2].port() == 1339 ||
              peers_bot1[2].port() == 1340);
  ASSERT_NE(peers_bot1[0].port(), peers_bot1[1].port());
  ASSERT_NE(peers_bot1[1].port(), peers_bot1[2].port());
  ASSERT_NE(peers_bot1[0].port(), peers_bot1[2].port());

  ASSERT_EQ(peers_bot2[0].endpoint(), "127.0.0.1");
  ASSERT_EQ(peers_bot2[1].endpoint(), "127.0.0.1");
  ASSERT_EQ(peers_bot2[2].endpoint(), "127.0.0.1");
  ASSERT_TRUE(peers_bot2[0].port() == 1337 || peers_bot2[0].port() == 1338 ||
              peers_bot2[0].port() == 1340);
  ASSERT_TRUE(peers_bot2[1].port() == 1337 || peers_bot2[1].port() == 1338 ||
              peers_bot2[1].port() == 1340);
  ASSERT_TRUE(peers_bot2[2].port() == 1337 || peers_bot2[2].port() == 1338 ||
              peers_bot2[2].port() == 1340);
  ASSERT_NE(peers_bot2[0].port(), peers_bot2[1].port());
  ASSERT_NE(peers_bot2[1].port(), peers_bot2[2].port());
  ASSERT_NE(peers_bot2[0].port(), peers_bot2[2].port());

  ASSERT_EQ(peers_bot3[0].endpoint(), "127.0.0.1");
  ASSERT_EQ(peers_bot3[1].endpoint(), "127.0.0.1");
  ASSERT_EQ(peers_bot3[2].endpoint(), "127.0.0.1");
  ASSERT_TRUE(peers_bot3[0].port() == 1337 || peers_bot3[0].port() == 1338 ||
              peers_bot3[0].port() == 1339);
  ASSERT_TRUE(peers_bot3[1].port() == 1337 || peers_bot3[1].port() == 1338 ||
              peers_bot3[1].port() == 1339);
  ASSERT_TRUE(peers_bot3[2].port() == 1337 || peers_bot3[2].port() == 1338 ||
              peers_bot3[2].port() == 1339);
  ASSERT_NE(peers_bot3[0].port(), peers_bot3[1].port());
  ASSERT_NE(peers_bot3[1].port(), peers_bot3[2].port());
  ASSERT_NE(peers_bot3[0].port(), peers_bot3[2].port());

  auto peers_bot4 = vectorize(bot4->connected_peers());

  ASSERT_EQ(peers_bot4.size(), 0);

  // Destroy a connected bot.
  bot3.reset();

  std::this_thread::sleep_for(5s);

  peers_bot0 = vectorize(bot0->connected_peers());
  peers_bot1 = vectorize(bot1->connected_peers());
  peers_bot2 = vectorize(bot2->connected_peers());
  peers_bot4 = vectorize(bot4->connected_peers());

  ASSERT_EQ(peers_bot0.size(), peers_bot1.size());
  ASSERT_EQ(peers_bot1.size(), peers_bot2.size());
  ASSERT_EQ(peers_bot2.size(), peers_bot4.size());
  ASSERT_EQ(peers_bot4.size(), 3);

  ASSERT_EQ(peers_bot0[0].endpoint(), "127.0.0.1");
  ASSERT_EQ(peers_bot0[1].endpoint(), "127.0.0.1");
  ASSERT_EQ(peers_bot0[2].endpoint(), "127.0.0.1");
  ASSERT_TRUE(peers_bot0[0].port() == 1338 || peers_bot0[0].port() == 1339 ||
              peers_bot0[0].port() == 1341);
  ASSERT_TRUE(peers_bot0[1].port() == 1338 || peers_bot0[1].port() == 1339 ||
              peers_bot0[1].port() == 1341);
  ASSERT_TRUE(peers_bot0[2].port() == 1338 || peers_bot0[2].port() == 1339 ||
              peers_bot0[2].port() == 1341);
  ASSERT_NE(peers_bot0[0].port(), peers_bot0[1].port());
  ASSERT_NE(peers_bot0[1].port(), peers_bot0[2].port());
  ASSERT_NE(peers_bot0[0].port(), peers_bot0[2].port());

  ASSERT_EQ(peers_bot1[0].endpoint(), "127.0.0.1");
  ASSERT_EQ(peers_bot1[1].endpoint(), "127.0.0.1");
  ASSERT_EQ(peers_bot1[2].endpoint(), "127.0.0.1");
  ASSERT_TRUE(peers_bot1[0].port() == 1337 || peers_bot1[0].port() == 1339 ||
              peers_bot1[0].port() == 1341);
  ASSERT_TRUE(peers_bot1[1].port() == 1337 || peers_bot1[1].port() == 1339 ||
              peers_bot1[1].port() == 1341);
  ASSERT_TRUE(peers_bot1[2].port() == 1337 || peers_bot1[2].port() == 1339 ||
              peers_bot1[2].port() == 1341);
  ASSERT_NE(peers_bot1[0].port(), peers_bot1[1].port());
  ASSERT_NE(peers_bot1[1].port(), peers_bot1[2].port());
  ASSERT_NE(peers_bot1[0].port(), peers_bot1[2].port());

  ASSERT_EQ(peers_bot2[0].endpoint(), "127.0.0.1");
  ASSERT_EQ(peers_bot2[1].endpoint(), "127.0.0.1");
  ASSERT_EQ(peers_bot2[2].endpoint(), "127.0.0.1");
  ASSERT_TRUE(peers_bot2[0].port() == 1337 || peers_bot2[0].port() == 1338 ||
              peers_bot2[0].port() == 1341);
  ASSERT_TRUE(peers_bot2[1].port() == 1337 || peers_bot2[1].port() == 1338 ||
              peers_bot2[1].port() == 1341);
  ASSERT_TRUE(peers_bot2[2].port() == 1337 || peers_bot2[2].port() == 1338 ||
              peers_bot2[2].port() == 1341);
  ASSERT_NE(peers_bot2[0].port(), peers_bot2[1].port());
  ASSERT_NE(peers_bot2[1].port(), peers_bot2[2].port());
  ASSERT_NE(peers_bot2[0].port(), peers_bot2[2].port());

  ASSERT_EQ(peers_bot4[0].endpoint(), "127.0.0.1");
  ASSERT_EQ(peers_bot4[1].endpoint(), "127.0.0.1");
  ASSERT_EQ(peers_bot4[2].endpoint(), "127.0.0.1");
  ASSERT_TRUE(peers_bot4[0].port() == 1337 || peers_bot4[0].port() == 1338 ||
              peers_bot4[0].port() == 1339);
  ASSERT_TRUE(peers_bot4[1].port() == 1337 || peers_bot4[1].port() == 1338 ||
              peers_bot4[1].port() == 1339);
  ASSERT_TRUE(peers_bot4[2].port() == 1337 || peers_bot4[2].port() == 1338 ||
              peers_bot4[2].port() == 1339);
  ASSERT_NE(peers_bot4[0].port(), peers_bot4[1].port());
  ASSERT_NE(peers_bot4[1].port(), peers_bot4[2].port());
  ASSERT_NE(peers_bot4[0].port(), peers_bot4[2].port());
}

TEST(INTEGRATION, connection_opportunity_2) {
  // Check a node excluded from a connected graph can connect after a node of
  // the compleete graph is destroyed. Check new node can connect to nodes it
  // did not have on its configuration file.
  Path config_path0("bot0.json");
  messages::config::Config config0(config_path0);
  auto bot0 = std::make_shared<Bot>(config0);
  Path config_path1("bot1.json");
  messages::config::Config config1(config_path1);
  auto bot1 = std::make_shared<Bot>(config1);
  Path config_path2("bot2.json");
  messages::config::Config config2(config_path2);
  auto bot2 = std::make_shared<Bot>(config2);
  Path config_path3("bot_outsider.40-38.39.json");
  messages::config::Config config3(config_path3);
  auto bot3 = std::make_shared<Bot>(config3);

  std::this_thread::sleep_for(5s);

  auto peers_bot0 = vectorize(bot0->connected_peers());
  auto peers_bot1 = vectorize(bot1->connected_peers());
  auto peers_bot2 = vectorize(bot2->connected_peers());
  auto peers_bot3 = vectorize(bot3->connected_peers());

  ASSERT_EQ(peers_bot0.size(), peers_bot1.size());
  ASSERT_EQ(peers_bot1.size(), peers_bot2.size());
  ASSERT_EQ(peers_bot2.size(), peers_bot3.size());
  ASSERT_EQ(peers_bot3.size(), 3);

  ASSERT_EQ(peers_bot0[0].endpoint(), "127.0.0.1");
  ASSERT_EQ(peers_bot0[1].endpoint(), "127.0.0.1");
  ASSERT_EQ(peers_bot0[2].endpoint(), "127.0.0.1");
  ASSERT_TRUE(peers_bot0[0].port() == 1338 || peers_bot0[0].port() == 1339 ||
              peers_bot0[0].port() == 1340);
  ASSERT_TRUE(peers_bot0[1].port() == 1338 || peers_bot0[1].port() == 1339 ||
              peers_bot0[1].port() == 1340);
  ASSERT_TRUE(peers_bot0[2].port() == 1338 || peers_bot0[2].port() == 1339 ||
              peers_bot0[2].port() == 1340);
  ASSERT_NE(peers_bot0[0].port(), peers_bot0[1].port());
  ASSERT_NE(peers_bot0[1].port(), peers_bot0[2].port());
  ASSERT_NE(peers_bot0[0].port(), peers_bot0[2].port());

  ASSERT_EQ(peers_bot1[0].endpoint(), "127.0.0.1");
  ASSERT_EQ(peers_bot1[1].endpoint(), "127.0.0.1");
  ASSERT_EQ(peers_bot1[2].endpoint(), "127.0.0.1");
  ASSERT_TRUE(peers_bot1[0].port() == 1337 || peers_bot1[0].port() == 1339 ||
              peers_bot1[0].port() == 1340);
  ASSERT_TRUE(peers_bot1[1].port() == 1337 || peers_bot1[1].port() == 1339 ||
              peers_bot1[1].port() == 1340);
  ASSERT_TRUE(peers_bot1[2].port() == 1337 || peers_bot1[2].port() == 1339 ||
              peers_bot1[2].port() == 1340);
  ASSERT_NE(peers_bot1[0].port(), peers_bot1[1].port());
  ASSERT_NE(peers_bot1[1].port(), peers_bot1[2].port());
  ASSERT_NE(peers_bot1[0].port(), peers_bot1[2].port());

  ASSERT_EQ(peers_bot2[0].endpoint(), "127.0.0.1");
  ASSERT_EQ(peers_bot2[1].endpoint(), "127.0.0.1");
  ASSERT_EQ(peers_bot2[2].endpoint(), "127.0.0.1");
  ASSERT_TRUE(peers_bot2[0].port() == 1337 || peers_bot2[0].port() == 1338 ||
              peers_bot2[0].port() == 1340);
  ASSERT_TRUE(peers_bot2[1].port() == 1337 || peers_bot2[1].port() == 1338 ||
              peers_bot2[1].port() == 1340);
  ASSERT_TRUE(peers_bot2[2].port() == 1337 || peers_bot2[2].port() == 1338 ||
              peers_bot2[2].port() == 1340);
  ASSERT_NE(peers_bot2[0].port(), peers_bot2[1].port());
  ASSERT_NE(peers_bot2[1].port(), peers_bot2[2].port());
  ASSERT_NE(peers_bot2[0].port(), peers_bot2[2].port());

  ASSERT_EQ(peers_bot3[0].endpoint(), "127.0.0.1");
  ASSERT_EQ(peers_bot3[1].endpoint(), "127.0.0.1");
  ASSERT_EQ(peers_bot3[2].endpoint(), "127.0.0.1");
  ASSERT_TRUE(peers_bot3[0].port() == 1337 || peers_bot3[0].port() == 1338 ||
              peers_bot3[0].port() == 1339);
  ASSERT_TRUE(peers_bot3[1].port() == 1337 || peers_bot3[1].port() == 1338 ||
              peers_bot3[1].port() == 1339);
  ASSERT_TRUE(peers_bot3[2].port() == 1337 || peers_bot3[2].port() == 1338 ||
              peers_bot3[2].port() == 1339);
  ASSERT_NE(peers_bot3[0].port(), peers_bot3[1].port());
  ASSERT_NE(peers_bot3[1].port(), peers_bot3[2].port());
  ASSERT_NE(peers_bot3[0].port(), peers_bot3[2].port());

  // Create additional node that cannot connect
  Path config_path4("bot_outsider.41-38.39.json");
  messages::config::Config config4(config_path4);
  auto bot4 = std::make_shared<Bot>(config4);

  std::this_thread::sleep_for(5s);

  // Check no change on other nodes
  peers_bot0 = vectorize(bot0->connected_peers());
  peers_bot1 = vectorize(bot1->connected_peers());
  peers_bot2 = vectorize(bot2->connected_peers());
  peers_bot3 = vectorize(bot3->connected_peers());

  ASSERT_EQ(peers_bot0.size(), peers_bot1.size());
  ASSERT_EQ(peers_bot1.size(), peers_bot2.size());
  ASSERT_EQ(peers_bot2.size(), peers_bot3.size());
  ASSERT_EQ(peers_bot3.size(), 3);

  ASSERT_EQ(peers_bot0[0].endpoint(), "127.0.0.1");
  ASSERT_EQ(peers_bot0[1].endpoint(), "127.0.0.1");
  ASSERT_EQ(peers_bot0[2].endpoint(), "127.0.0.1");
  ASSERT_TRUE(peers_bot0[0].port() == 1338 || peers_bot0[0].port() == 1339 ||
              peers_bot0[0].port() == 1340);
  ASSERT_TRUE(peers_bot0[1].port() == 1338 || peers_bot0[1].port() == 1339 ||
              peers_bot0[1].port() == 1340);
  ASSERT_TRUE(peers_bot0[2].port() == 1338 || peers_bot0[2].port() == 1339 ||
              peers_bot0[2].port() == 1340);
  ASSERT_NE(peers_bot0[0].port(), peers_bot0[1].port());
  ASSERT_NE(peers_bot0[1].port(), peers_bot0[2].port());
  ASSERT_NE(peers_bot0[0].port(), peers_bot0[2].port());

  ASSERT_EQ(peers_bot1[0].endpoint(), "127.0.0.1");
  ASSERT_EQ(peers_bot1[1].endpoint(), "127.0.0.1");
  ASSERT_EQ(peers_bot1[2].endpoint(), "127.0.0.1");
  ASSERT_TRUE(peers_bot1[0].port() == 1337 || peers_bot1[0].port() == 1339 ||
              peers_bot1[0].port() == 1340);
  ASSERT_TRUE(peers_bot1[1].port() == 1337 || peers_bot1[1].port() == 1339 ||
              peers_bot1[1].port() == 1340);
  ASSERT_TRUE(peers_bot1[2].port() == 1337 || peers_bot1[2].port() == 1339 ||
              peers_bot1[2].port() == 1340);
  ASSERT_NE(peers_bot1[0].port(), peers_bot1[1].port());
  ASSERT_NE(peers_bot1[1].port(), peers_bot1[2].port());
  ASSERT_NE(peers_bot1[0].port(), peers_bot1[2].port());

  ASSERT_EQ(peers_bot2[0].endpoint(), "127.0.0.1");
  ASSERT_EQ(peers_bot2[1].endpoint(), "127.0.0.1");
  ASSERT_EQ(peers_bot2[2].endpoint(), "127.0.0.1");
  ASSERT_TRUE(peers_bot2[0].port() == 1337 || peers_bot2[0].port() == 1338 ||
              peers_bot2[0].port() == 1340);
  ASSERT_TRUE(peers_bot2[1].port() == 1337 || peers_bot2[1].port() == 1338 ||
              peers_bot2[1].port() == 1340);
  ASSERT_TRUE(peers_bot2[2].port() == 1337 || peers_bot2[2].port() == 1338 ||
              peers_bot2[2].port() == 1340);
  ASSERT_NE(peers_bot2[0].port(), peers_bot2[1].port());
  ASSERT_NE(peers_bot2[1].port(), peers_bot2[2].port());
  ASSERT_NE(peers_bot2[0].port(), peers_bot2[2].port());

  ASSERT_EQ(peers_bot3[0].endpoint(), "127.0.0.1");
  ASSERT_EQ(peers_bot3[1].endpoint(), "127.0.0.1");
  ASSERT_EQ(peers_bot3[2].endpoint(), "127.0.0.1");
  ASSERT_TRUE(peers_bot3[0].port() == 1337 || peers_bot3[0].port() == 1338 ||
              peers_bot3[0].port() == 1339);
  ASSERT_TRUE(peers_bot3[1].port() == 1337 || peers_bot3[1].port() == 1338 ||
              peers_bot3[1].port() == 1339);
  ASSERT_TRUE(peers_bot3[2].port() == 1337 || peers_bot3[2].port() == 1338 ||
              peers_bot3[2].port() == 1339);
  ASSERT_NE(peers_bot3[0].port(), peers_bot3[1].port());
  ASSERT_NE(peers_bot3[1].port(), peers_bot3[2].port());
  ASSERT_NE(peers_bot3[0].port(), peers_bot3[2].port());

  auto peers_bot4 = vectorize(bot4->connected_peers());

  ASSERT_EQ(peers_bot4.size(), 0);

  // Destroy a connected bot.
  bot1.reset();

  std::this_thread::sleep_for(5s);

  peers_bot0 = vectorize(bot0->connected_peers());
  peers_bot3 = vectorize(bot3->connected_peers());
  peers_bot2 = vectorize(bot2->connected_peers());
  peers_bot4 = vectorize(bot4->connected_peers());

  ASSERT_EQ(peers_bot0.size(), peers_bot3.size());
  ASSERT_EQ(peers_bot3.size(), peers_bot2.size());
  ASSERT_EQ(peers_bot2.size(), peers_bot4.size());
  ASSERT_EQ(peers_bot4.size(), 3);

  ASSERT_EQ(peers_bot0[0].endpoint(), "127.0.0.1");
  ASSERT_EQ(peers_bot0[1].endpoint(), "127.0.0.1");
  ASSERT_EQ(peers_bot0[2].endpoint(), "127.0.0.1");
  ASSERT_TRUE(peers_bot0[0].port() == 1340 || peers_bot0[0].port() == 1339 ||
              peers_bot0[0].port() == 1341);
  ASSERT_TRUE(peers_bot0[1].port() == 1340 || peers_bot0[1].port() == 1339 ||
              peers_bot0[1].port() == 1341);
  ASSERT_TRUE(peers_bot0[2].port() == 1340 || peers_bot0[2].port() == 1339 ||
              peers_bot0[2].port() == 1341);
  ASSERT_NE(peers_bot0[0].port(), peers_bot0[1].port());
  ASSERT_NE(peers_bot0[1].port(), peers_bot0[2].port());
  ASSERT_NE(peers_bot0[0].port(), peers_bot0[2].port());

  ASSERT_EQ(peers_bot3[0].endpoint(), "127.0.0.1");
  ASSERT_EQ(peers_bot3[1].endpoint(), "127.0.0.1");
  ASSERT_EQ(peers_bot3[2].endpoint(), "127.0.0.1");
  ASSERT_TRUE(peers_bot3[0].port() == 1337 || peers_bot3[0].port() == 1339 ||
              peers_bot3[0].port() == 1341);
  ASSERT_TRUE(peers_bot3[1].port() == 1337 || peers_bot3[1].port() == 1339 ||
              peers_bot3[1].port() == 1341);
  ASSERT_TRUE(peers_bot3[2].port() == 1337 || peers_bot3[2].port() == 1339 ||
              peers_bot3[2].port() == 1341);
  ASSERT_NE(peers_bot3[0].port(), peers_bot3[1].port());
  ASSERT_NE(peers_bot3[1].port(), peers_bot3[2].port());
  ASSERT_NE(peers_bot3[0].port(), peers_bot3[2].port());

  ASSERT_EQ(peers_bot2[0].endpoint(), "127.0.0.1");
  ASSERT_EQ(peers_bot2[1].endpoint(), "127.0.0.1");
  ASSERT_EQ(peers_bot2[2].endpoint(), "127.0.0.1");
  ASSERT_TRUE(peers_bot2[0].port() == 1337 || peers_bot2[0].port() == 1340 ||
              peers_bot2[0].port() == 1341);
  ASSERT_TRUE(peers_bot2[1].port() == 1337 || peers_bot2[1].port() == 1340 ||
              peers_bot2[1].port() == 1341);
  ASSERT_TRUE(peers_bot2[2].port() == 1337 || peers_bot2[2].port() == 1340 ||
              peers_bot2[2].port() == 1341);
  ASSERT_NE(peers_bot2[0].port(), peers_bot2[1].port());
  ASSERT_NE(peers_bot2[1].port(), peers_bot2[2].port());
  ASSERT_NE(peers_bot2[0].port(), peers_bot2[2].port());

  ASSERT_EQ(peers_bot4[0].endpoint(), "127.0.0.1");
  ASSERT_EQ(peers_bot4[1].endpoint(), "127.0.0.1");
  ASSERT_EQ(peers_bot4[2].endpoint(), "127.0.0.1");
  ASSERT_TRUE(peers_bot4[0].port() == 1337 || peers_bot4[0].port() == 1340 ||
              peers_bot4[0].port() == 1339);
  ASSERT_TRUE(peers_bot4[1].port() == 1337 || peers_bot4[1].port() == 1340 ||
              peers_bot4[1].port() == 1339);
  ASSERT_TRUE(peers_bot4[2].port() == 1337 || peers_bot4[2].port() == 1340 ||
              peers_bot4[2].port() == 1339);
  ASSERT_NE(peers_bot4[0].port(), peers_bot4[1].port());
  ASSERT_NE(peers_bot4[1].port(), peers_bot4[2].port());
  ASSERT_NE(peers_bot4[0].port(), peers_bot4[2].port());
}

TEST(INTEGRATION, connection_opportunity_update) {
  // Check a node excluded from a connected graph can connect after a node of
  // the compleete graph is destroyed even if the only node it knows is full.
  Path config_path0("bot0.json");
  messages::config::Config config0(config_path0);
  auto bot0 = std::make_shared<Bot>(config0);
  Path config_path1("bot1.json");
  messages::config::Config config1(config_path1);
  auto bot1 = std::make_shared<Bot>(config1);
  Path config_path2("bot2.json");
  messages::config::Config config2(config_path2);
  auto bot2 = std::make_shared<Bot>(config2);
  Path config_path3("bot_outsider.40-38.39.json");
  messages::config::Config config3(config_path3);
  auto bot3 = std::make_shared<Bot>(config3);

  std::this_thread::sleep_for(10s);

  auto peers_bot0 = vectorize(bot0->connected_peers());
  auto peers_bot1 = vectorize(bot1->connected_peers());
  auto peers_bot2 = vectorize(bot2->connected_peers());
  auto peers_bot3 = vectorize(bot3->connected_peers());

  ASSERT_EQ(peers_bot0.size(), peers_bot1.size());
  ASSERT_EQ(peers_bot1.size(), peers_bot2.size());
  ASSERT_EQ(peers_bot2.size(), peers_bot3.size());
  ASSERT_EQ(peers_bot3.size(), 3);

  ASSERT_EQ(peers_bot0[0].endpoint(), "127.0.0.1");
  ASSERT_EQ(peers_bot0[1].endpoint(), "127.0.0.1");
  ASSERT_EQ(peers_bot0[2].endpoint(), "127.0.0.1");
  ASSERT_TRUE(peers_bot0[0].port() == 1338 || peers_bot0[0].port() == 1339 ||
              peers_bot0[0].port() == 1340);
  ASSERT_TRUE(peers_bot0[1].port() == 1338 || peers_bot0[1].port() == 1339 ||
              peers_bot0[1].port() == 1340);
  ASSERT_TRUE(peers_bot0[2].port() == 1338 || peers_bot0[2].port() == 1339 ||
              peers_bot0[2].port() == 1340);
  ASSERT_NE(peers_bot0[0].port(), peers_bot0[1].port());
  ASSERT_NE(peers_bot0[1].port(), peers_bot0[2].port());
  ASSERT_NE(peers_bot0[0].port(), peers_bot0[2].port());

  ASSERT_EQ(peers_bot1[0].endpoint(), "127.0.0.1");
  ASSERT_EQ(peers_bot1[1].endpoint(), "127.0.0.1");
  ASSERT_EQ(peers_bot1[2].endpoint(), "127.0.0.1");
  ASSERT_TRUE(peers_bot1[0].port() == 1337 || peers_bot1[0].port() == 1339 ||
              peers_bot1[0].port() == 1340);
  ASSERT_TRUE(peers_bot1[1].port() == 1337 || peers_bot1[1].port() == 1339 ||
              peers_bot1[1].port() == 1340);
  ASSERT_TRUE(peers_bot1[2].port() == 1337 || peers_bot1[2].port() == 1339 ||
              peers_bot1[2].port() == 1340);
  ASSERT_NE(peers_bot1[0].port(), peers_bot1[1].port());
  ASSERT_NE(peers_bot1[1].port(), peers_bot1[2].port());
  ASSERT_NE(peers_bot1[0].port(), peers_bot1[2].port());

  ASSERT_EQ(peers_bot2[0].endpoint(), "127.0.0.1");
  ASSERT_EQ(peers_bot2[1].endpoint(), "127.0.0.1");
  ASSERT_EQ(peers_bot2[2].endpoint(), "127.0.0.1");
  ASSERT_TRUE(peers_bot2[0].port() == 1337 || peers_bot2[0].port() == 1338 ||
              peers_bot2[0].port() == 1340);
  ASSERT_TRUE(peers_bot2[1].port() == 1337 || peers_bot2[1].port() == 1338 ||
              peers_bot2[1].port() == 1340);
  ASSERT_TRUE(peers_bot2[2].port() == 1337 || peers_bot2[2].port() == 1338 ||
              peers_bot2[2].port() == 1340);
  ASSERT_NE(peers_bot2[0].port(), peers_bot2[1].port());
  ASSERT_NE(peers_bot2[1].port(), peers_bot2[2].port());
  ASSERT_NE(peers_bot2[0].port(), peers_bot2[2].port());

  ASSERT_EQ(peers_bot3[0].endpoint(), "127.0.0.1");
  ASSERT_EQ(peers_bot3[1].endpoint(), "127.0.0.1");
  ASSERT_EQ(peers_bot3[2].endpoint(), "127.0.0.1");
  ASSERT_TRUE(peers_bot3[0].port() == 1337 || peers_bot3[0].port() == 1338 ||
              peers_bot3[0].port() == 1339);
  ASSERT_TRUE(peers_bot3[1].port() == 1337 || peers_bot3[1].port() == 1338 ||
              peers_bot3[1].port() == 1339);
  ASSERT_TRUE(peers_bot3[2].port() == 1337 || peers_bot3[2].port() == 1338 ||
              peers_bot3[2].port() == 1339);
  ASSERT_NE(peers_bot3[0].port(), peers_bot3[1].port());
  ASSERT_NE(peers_bot3[1].port(), peers_bot3[2].port());
  ASSERT_NE(peers_bot3[0].port(), peers_bot3[2].port());

  // Create additional node that cannot connect
  Path config_path4("bot_outsider.50-37.json");
  messages::config::Config config4(config_path4);
  auto bot4 = std::make_shared<Bot>(config4);

  std::this_thread::sleep_for(10s);

  // Check no change on other nodes
  peers_bot0 = vectorize(bot0->connected_peers());
  peers_bot1 = vectorize(bot1->connected_peers());
  peers_bot2 = vectorize(bot2->connected_peers());
  peers_bot3 = vectorize(bot3->connected_peers());

  ASSERT_EQ(peers_bot0.size(), peers_bot1.size());
  ASSERT_EQ(peers_bot1.size(), peers_bot2.size());
  ASSERT_EQ(peers_bot2.size(), peers_bot3.size());
  ASSERT_EQ(peers_bot3.size(), 3);

  ASSERT_EQ(peers_bot0[0].endpoint(), "127.0.0.1");
  ASSERT_EQ(peers_bot0[1].endpoint(), "127.0.0.1");
  ASSERT_EQ(peers_bot0[2].endpoint(), "127.0.0.1");
  ASSERT_TRUE(peers_bot0[0].port() == 1338 || peers_bot0[0].port() == 1339 ||
              peers_bot0[0].port() == 1340);
  ASSERT_TRUE(peers_bot0[1].port() == 1338 || peers_bot0[1].port() == 1339 ||
              peers_bot0[1].port() == 1340);
  ASSERT_TRUE(peers_bot0[2].port() == 1338 || peers_bot0[2].port() == 1339 ||
              peers_bot0[2].port() == 1340);
  ASSERT_NE(peers_bot0[0].port(), peers_bot0[1].port());
  ASSERT_NE(peers_bot0[1].port(), peers_bot0[2].port());
  ASSERT_NE(peers_bot0[0].port(), peers_bot0[2].port());

  ASSERT_EQ(peers_bot1[0].endpoint(), "127.0.0.1");
  ASSERT_EQ(peers_bot1[1].endpoint(), "127.0.0.1");
  ASSERT_EQ(peers_bot1[2].endpoint(), "127.0.0.1");
  ASSERT_TRUE(peers_bot1[0].port() == 1337 || peers_bot1[0].port() == 1339 ||
              peers_bot1[0].port() == 1340);
  ASSERT_TRUE(peers_bot1[1].port() == 1337 || peers_bot1[1].port() == 1339 ||
              peers_bot1[1].port() == 1340);
  ASSERT_TRUE(peers_bot1[2].port() == 1337 || peers_bot1[2].port() == 1339 ||
              peers_bot1[2].port() == 1340);
  ASSERT_NE(peers_bot1[0].port(), peers_bot1[1].port());
  ASSERT_NE(peers_bot1[1].port(), peers_bot1[2].port());
  ASSERT_NE(peers_bot1[0].port(), peers_bot1[2].port());

  ASSERT_EQ(peers_bot2[0].endpoint(), "127.0.0.1");
  ASSERT_EQ(peers_bot2[1].endpoint(), "127.0.0.1");
  ASSERT_EQ(peers_bot2[2].endpoint(), "127.0.0.1");
  ASSERT_TRUE(peers_bot2[0].port() == 1337 || peers_bot2[0].port() == 1338 ||
              peers_bot2[0].port() == 1340);
  ASSERT_TRUE(peers_bot2[1].port() == 1337 || peers_bot2[1].port() == 1338 ||
              peers_bot2[1].port() == 1340);
  ASSERT_TRUE(peers_bot2[2].port() == 1337 || peers_bot2[2].port() == 1338 ||
              peers_bot2[2].port() == 1340);
  ASSERT_NE(peers_bot2[0].port(), peers_bot2[1].port());
  ASSERT_NE(peers_bot2[1].port(), peers_bot2[2].port());
  ASSERT_NE(peers_bot2[0].port(), peers_bot2[2].port());

  ASSERT_EQ(peers_bot3[0].endpoint(), "127.0.0.1");
  ASSERT_EQ(peers_bot3[1].endpoint(), "127.0.0.1");
  ASSERT_EQ(peers_bot3[2].endpoint(), "127.0.0.1");
  ASSERT_TRUE(peers_bot3[0].port() == 1337 || peers_bot3[0].port() == 1338 ||
              peers_bot3[0].port() == 1339);
  ASSERT_TRUE(peers_bot3[1].port() == 1337 || peers_bot3[1].port() == 1338 ||
              peers_bot3[1].port() == 1339);
  ASSERT_TRUE(peers_bot3[2].port() == 1337 || peers_bot3[2].port() == 1338 ||
              peers_bot3[2].port() == 1339);
  ASSERT_NE(peers_bot3[0].port(), peers_bot3[1].port());
  ASSERT_NE(peers_bot3[1].port(), peers_bot3[2].port());
  ASSERT_NE(peers_bot3[0].port(), peers_bot3[2].port());

  auto peers_bot4 = vectorize(bot4->connected_peers());

  ASSERT_EQ(peers_bot4.size(), 0);

  // Destroy a connected bot.
  bot3.reset();

  std::this_thread::sleep_for(10s);

  peers_bot0 = vectorize(bot0->connected_peers());
  peers_bot1 = vectorize(bot1->connected_peers());
  peers_bot2 = vectorize(bot2->connected_peers());
  peers_bot4 = vectorize(bot4->connected_peers());

  ASSERT_EQ(peers_bot0.size(), peers_bot1.size());
  ASSERT_EQ(peers_bot1.size(), peers_bot2.size());
  ASSERT_EQ(peers_bot2.size(), peers_bot4.size());
  ASSERT_EQ(peers_bot4.size(), 3);

  ASSERT_EQ(peers_bot0[0].endpoint(), "127.0.0.1");
  ASSERT_EQ(peers_bot0[1].endpoint(), "127.0.0.1");
  ASSERT_EQ(peers_bot0[2].endpoint(), "127.0.0.1");
  ASSERT_TRUE(peers_bot0[0].port() == 1338 || peers_bot0[0].port() == 1339 ||
              peers_bot0[0].port() == 1350);
  ASSERT_TRUE(peers_bot0[1].port() == 1338 || peers_bot0[1].port() == 1339 ||
              peers_bot0[1].port() == 1350);
  ASSERT_TRUE(peers_bot0[2].port() == 1338 || peers_bot0[2].port() == 1339 ||
              peers_bot0[2].port() == 1350);
  ASSERT_NE(peers_bot0[0].port(), peers_bot0[1].port());
  ASSERT_NE(peers_bot0[1].port(), peers_bot0[2].port());
  ASSERT_NE(peers_bot0[0].port(), peers_bot0[2].port());

  ASSERT_EQ(peers_bot1[0].endpoint(), "127.0.0.1");
  ASSERT_EQ(peers_bot1[1].endpoint(), "127.0.0.1");
  ASSERT_EQ(peers_bot1[2].endpoint(), "127.0.0.1");
  ASSERT_TRUE(peers_bot1[0].port() == 1337 || peers_bot1[0].port() == 1339 ||
              peers_bot1[0].port() == 1350);
  ASSERT_TRUE(peers_bot1[1].port() == 1337 || peers_bot1[1].port() == 1339 ||
              peers_bot1[1].port() == 1350);
  ASSERT_TRUE(peers_bot1[2].port() == 1337 || peers_bot1[2].port() == 1339 ||
              peers_bot1[2].port() == 1350);
  ASSERT_NE(peers_bot1[0].port(), peers_bot1[1].port());
  ASSERT_NE(peers_bot1[1].port(), peers_bot1[2].port());
  ASSERT_NE(peers_bot1[0].port(), peers_bot1[2].port());

  ASSERT_EQ(peers_bot2[0].endpoint(), "127.0.0.1");
  ASSERT_EQ(peers_bot2[1].endpoint(), "127.0.0.1");
  ASSERT_EQ(peers_bot2[2].endpoint(), "127.0.0.1");
  ASSERT_TRUE(peers_bot2[0].port() == 1337 || peers_bot2[0].port() == 1338 ||
              peers_bot2[0].port() == 1350);
  ASSERT_TRUE(peers_bot2[1].port() == 1337 || peers_bot2[1].port() == 1338 ||
              peers_bot2[1].port() == 1350);
  ASSERT_TRUE(peers_bot2[2].port() == 1337 || peers_bot2[2].port() == 1338 ||
              peers_bot2[2].port() == 1350);
  ASSERT_NE(peers_bot2[0].port(), peers_bot2[1].port());
  ASSERT_NE(peers_bot2[1].port(), peers_bot2[2].port());
  ASSERT_NE(peers_bot2[0].port(), peers_bot2[2].port());

  ASSERT_EQ(peers_bot4[0].endpoint(), "127.0.0.1");
  ASSERT_EQ(peers_bot4[1].endpoint(), "127.0.0.1");
  ASSERT_EQ(peers_bot4[2].endpoint(), "127.0.0.1");
  ASSERT_TRUE(peers_bot4[0].port() == 1337 || peers_bot4[0].port() == 1338 ||
              peers_bot4[0].port() == 1339);
  ASSERT_TRUE(peers_bot4[1].port() == 1337 || peers_bot4[1].port() == 1338 ||
              peers_bot4[1].port() == 1339);
  ASSERT_TRUE(peers_bot4[2].port() == 1337 || peers_bot4[2].port() == 1338 ||
              peers_bot4[2].port() == 1339);
  ASSERT_NE(peers_bot4[0].port(), peers_bot4[1].port());
  ASSERT_NE(peers_bot4[1].port(), peers_bot4[2].port());
  ASSERT_NE(peers_bot4[0].port(), peers_bot4[2].port());
}

TEST(INTEGRATION, connection_reconfig) {
  // Test that new nodes can form a graph even if first node are all
  // discinnected.
  Path config_path0("bot0.json");
  messages::config::Config config0(config_path0);
  auto bot0 = std::make_shared<Bot>(config0);
  Path config_path1("bot1.json");
  messages::config::Config config1(config_path1);
  auto bot1 = std::make_shared<Bot>(config1);
  Path config_path2("bot2.json");
  messages::config::Config config2(config_path2);
  auto bot2 = std::make_shared<Bot>(config2);
  Path config_path3("bot_outsider.40-38.39.json");
  messages::config::Config config3(config_path3);
  auto bot3 = std::make_shared<Bot>(config3);

  std::this_thread::sleep_for(5s);

  auto peers_bot0 = vectorize(bot0->connected_peers());
  auto peers_bot1 = vectorize(bot1->connected_peers());
  auto peers_bot2 = vectorize(bot2->connected_peers());
  auto peers_bot3 = vectorize(bot3->connected_peers());

  ASSERT_EQ(peers_bot0.size(), peers_bot1.size());
  ASSERT_EQ(peers_bot1.size(), peers_bot2.size());
  ASSERT_EQ(peers_bot2.size(), peers_bot3.size());
  ASSERT_EQ(peers_bot3.size(), 3);

  ASSERT_EQ(peers_bot0[0].endpoint(), "127.0.0.1");
  ASSERT_EQ(peers_bot0[1].endpoint(), "127.0.0.1");
  ASSERT_EQ(peers_bot0[2].endpoint(), "127.0.0.1");
  ASSERT_TRUE(peers_bot0[0].port() == 1338 || peers_bot0[0].port() == 1339 ||
              peers_bot0[0].port() == 1340);
  ASSERT_TRUE(peers_bot0[1].port() == 1338 || peers_bot0[1].port() == 1339 ||
              peers_bot0[1].port() == 1340);
  ASSERT_TRUE(peers_bot0[2].port() == 1338 || peers_bot0[2].port() == 1339 ||
              peers_bot0[2].port() == 1340);
  ASSERT_NE(peers_bot0[0].port(), peers_bot0[1].port());
  ASSERT_NE(peers_bot0[1].port(), peers_bot0[2].port());
  ASSERT_NE(peers_bot0[0].port(), peers_bot0[2].port());

  ASSERT_EQ(peers_bot1[0].endpoint(), "127.0.0.1");
  ASSERT_EQ(peers_bot1[1].endpoint(), "127.0.0.1");
  ASSERT_EQ(peers_bot1[2].endpoint(), "127.0.0.1");
  ASSERT_TRUE(peers_bot1[0].port() == 1337 || peers_bot1[0].port() == 1339 ||
              peers_bot1[0].port() == 1340);
  ASSERT_TRUE(peers_bot1[1].port() == 1337 || peers_bot1[1].port() == 1339 ||
              peers_bot1[1].port() == 1340);
  ASSERT_TRUE(peers_bot1[2].port() == 1337 || peers_bot1[2].port() == 1339 ||
              peers_bot1[2].port() == 1340);
  ASSERT_NE(peers_bot1[0].port(), peers_bot1[1].port());
  ASSERT_NE(peers_bot1[1].port(), peers_bot1[2].port());
  ASSERT_NE(peers_bot1[0].port(), peers_bot1[2].port());

  ASSERT_EQ(peers_bot2[0].endpoint(), "127.0.0.1");
  ASSERT_EQ(peers_bot2[1].endpoint(), "127.0.0.1");
  ASSERT_EQ(peers_bot2[2].endpoint(), "127.0.0.1");
  ASSERT_TRUE(peers_bot2[0].port() == 1337 || peers_bot2[0].port() == 1338 ||
              peers_bot2[0].port() == 1340);
  ASSERT_TRUE(peers_bot2[1].port() == 1337 || peers_bot2[1].port() == 1338 ||
              peers_bot2[1].port() == 1340);
  ASSERT_TRUE(peers_bot2[2].port() == 1337 || peers_bot2[2].port() == 1338 ||
              peers_bot2[2].port() == 1340);
  ASSERT_NE(peers_bot2[0].port(), peers_bot2[1].port());
  ASSERT_NE(peers_bot2[1].port(), peers_bot2[2].port());
  ASSERT_NE(peers_bot2[0].port(), peers_bot2[2].port());

  ASSERT_EQ(peers_bot3[0].endpoint(), "127.0.0.1");
  ASSERT_EQ(peers_bot3[1].endpoint(), "127.0.0.1");
  ASSERT_EQ(peers_bot3[2].endpoint(), "127.0.0.1");
  ASSERT_TRUE(peers_bot3[0].port() == 1337 || peers_bot3[0].port() == 1338 ||
              peers_bot3[0].port() == 1339);
  ASSERT_TRUE(peers_bot3[1].port() == 1337 || peers_bot3[1].port() == 1338 ||
              peers_bot3[1].port() == 1339);
  ASSERT_TRUE(peers_bot3[2].port() == 1337 || peers_bot3[2].port() == 1338 ||
              peers_bot3[2].port() == 1339);
  ASSERT_NE(peers_bot3[0].port(), peers_bot3[1].port());
  ASSERT_NE(peers_bot3[1].port(), peers_bot3[2].port());
  ASSERT_NE(peers_bot3[0].port(), peers_bot3[2].port());

  // Now adding peers that know only onebot of the network.
  Path config_path4("bot_outsider.50-37.json");
  messages::config::Config config4(config_path4);
  auto bot4 = std::make_shared<Bot>(config4);
  Path config_path5("bot_outsider.51-37.json");
  messages::config::Config config5(config_path5);
  auto bot5 = std::make_shared<Bot>(config5);
  Path config_path6("bot_outsider.52-37.json");
  messages::config::Config config6(config_path6);
  auto bot6 = std::make_shared<Bot>(config6);

  std::this_thread::sleep_for(5s);

  // Check there is no change for currentnetwork.
  peers_bot0 = vectorize(bot0->connected_peers());
  peers_bot1 = vectorize(bot1->connected_peers());
  peers_bot2 = vectorize(bot2->connected_peers());
  peers_bot3 = vectorize(bot3->connected_peers());

  ASSERT_EQ(peers_bot0.size(), peers_bot1.size());
  ASSERT_EQ(peers_bot1.size(), peers_bot2.size());
  ASSERT_EQ(peers_bot2.size(), peers_bot3.size());
  ASSERT_EQ(peers_bot3.size(), 3);

  ASSERT_EQ(peers_bot0[0].endpoint(), "127.0.0.1");
  ASSERT_EQ(peers_bot0[1].endpoint(), "127.0.0.1");
  ASSERT_EQ(peers_bot0[2].endpoint(), "127.0.0.1");
  ASSERT_TRUE(peers_bot0[0].port() == 1338 || peers_bot0[0].port() == 1339 ||
              peers_bot0[0].port() == 1340);
  ASSERT_TRUE(peers_bot0[1].port() == 1338 || peers_bot0[1].port() == 1339 ||
              peers_bot0[1].port() == 1340);
  ASSERT_TRUE(peers_bot0[2].port() == 1338 || peers_bot0[2].port() == 1339 ||
              peers_bot0[2].port() == 1340);
  ASSERT_NE(peers_bot0[0].port(), peers_bot0[1].port());
  ASSERT_NE(peers_bot0[1].port(), peers_bot0[2].port());
  ASSERT_NE(peers_bot0[0].port(), peers_bot0[2].port());

  ASSERT_EQ(peers_bot1[0].endpoint(), "127.0.0.1");
  ASSERT_EQ(peers_bot1[1].endpoint(), "127.0.0.1");
  ASSERT_EQ(peers_bot1[2].endpoint(), "127.0.0.1");
  ASSERT_TRUE(peers_bot1[0].port() == 1337 || peers_bot1[0].port() == 1339 ||
              peers_bot1[0].port() == 1340);
  ASSERT_TRUE(peers_bot1[1].port() == 1337 || peers_bot1[1].port() == 1339 ||
              peers_bot1[1].port() == 1340);
  ASSERT_TRUE(peers_bot1[2].port() == 1337 || peers_bot1[2].port() == 1339 ||
              peers_bot1[2].port() == 1340);
  ASSERT_NE(peers_bot1[0].port(), peers_bot1[1].port());
  ASSERT_NE(peers_bot1[1].port(), peers_bot1[2].port());
  ASSERT_NE(peers_bot1[0].port(), peers_bot1[2].port());

  ASSERT_EQ(peers_bot2[0].endpoint(), "127.0.0.1");
  ASSERT_EQ(peers_bot2[1].endpoint(), "127.0.0.1");
  ASSERT_EQ(peers_bot2[2].endpoint(), "127.0.0.1");
  ASSERT_TRUE(peers_bot2[0].port() == 1337 || peers_bot2[0].port() == 1338 ||
              peers_bot2[0].port() == 1340);
  ASSERT_TRUE(peers_bot2[1].port() == 1337 || peers_bot2[1].port() == 1338 ||
              peers_bot2[1].port() == 1340);
  ASSERT_TRUE(peers_bot2[2].port() == 1337 || peers_bot2[2].port() == 1338 ||
              peers_bot2[2].port() == 1340);
  ASSERT_NE(peers_bot2[0].port(), peers_bot2[1].port());
  ASSERT_NE(peers_bot2[1].port(), peers_bot2[2].port());
  ASSERT_NE(peers_bot2[0].port(), peers_bot2[2].port());

  ASSERT_EQ(peers_bot3[0].endpoint(), "127.0.0.1");
  ASSERT_EQ(peers_bot3[1].endpoint(), "127.0.0.1");
  ASSERT_EQ(peers_bot3[2].endpoint(), "127.0.0.1");
  ASSERT_TRUE(peers_bot3[0].port() == 1337 || peers_bot3[0].port() == 1338 ||
              peers_bot3[0].port() == 1339);
  ASSERT_TRUE(peers_bot3[1].port() == 1337 || peers_bot3[1].port() == 1338 ||
              peers_bot3[1].port() == 1339);
  ASSERT_TRUE(peers_bot3[2].port() == 1337 || peers_bot3[2].port() == 1338 ||
              peers_bot3[2].port() == 1339);
  ASSERT_NE(peers_bot3[0].port(), peers_bot3[1].port());
  ASSERT_NE(peers_bot3[1].port(), peers_bot3[2].port());
  ASSERT_NE(peers_bot3[0].port(), peers_bot3[2].port());

  // And check none of the 3 laast bot is connected yet.
  auto peers_bot4 = vectorize(bot4->connected_peers());
  auto peers_bot5 = vectorize(bot5->connected_peers());
  auto peers_bot6 = vectorize(bot6->connected_peers());

  ASSERT_EQ(peers_bot4.size(), peers_bot5.size());
  ASSERT_EQ(peers_bot5.size(), peers_bot6.size());
  ASSERT_EQ(peers_bot6.size(), 0);

  // Delete 3 first bots.
  bot0.reset();
  bot1.reset();
  bot2.reset();

  std::this_thread::sleep_for(5s);

  // Check we get a new fully connected network.
  peers_bot3 = vectorize(bot3->connected_peers());
  peers_bot4 = vectorize(bot4->connected_peers());
  peers_bot5 = vectorize(bot5->connected_peers());
  peers_bot6 = vectorize(bot6->connected_peers());

  ASSERT_EQ(peers_bot3.size(), peers_bot4.size());
  ASSERT_EQ(peers_bot4.size(), peers_bot5.size());
  ASSERT_EQ(peers_bot5.size(), peers_bot6.size());
  ASSERT_EQ(peers_bot6.size(), 3);

  ASSERT_EQ(peers_bot4[0].endpoint(), "127.0.0.1");
  ASSERT_EQ(peers_bot4[1].endpoint(), "127.0.0.1");
  ASSERT_EQ(peers_bot4[2].endpoint(), "127.0.0.1");
  ASSERT_TRUE(peers_bot4[0].port() == 1340 || peers_bot4[0].port() == 1351 ||
              peers_bot4[0].port() == 1352);
  ASSERT_TRUE(peers_bot4[1].port() == 1340 || peers_bot4[1].port() == 1351 ||
              peers_bot4[1].port() == 1352);
  ASSERT_TRUE(peers_bot4[2].port() == 1340 || peers_bot4[2].port() == 1351 ||
              peers_bot4[2].port() == 1352);
  ASSERT_NE(peers_bot4[0].port(), peers_bot4[1].port());
  ASSERT_NE(peers_bot4[1].port(), peers_bot4[2].port());
  ASSERT_NE(peers_bot4[0].port(), peers_bot4[2].port());

  ASSERT_EQ(peers_bot5[0].endpoint(), "127.0.0.1");
  ASSERT_EQ(peers_bot5[1].endpoint(), "127.0.0.1");
  ASSERT_EQ(peers_bot5[2].endpoint(), "127.0.0.1");
  ASSERT_TRUE(peers_bot5[0].port() == 1350 || peers_bot5[0].port() == 1352 ||
              peers_bot5[0].port() == 1340);
  ASSERT_TRUE(peers_bot5[1].port() == 1350 || peers_bot5[1].port() == 1352 ||
              peers_bot5[1].port() == 1340);
  ASSERT_TRUE(peers_bot5[2].port() == 1350 || peers_bot5[2].port() == 1352 ||
              peers_bot5[2].port() == 1340);
  ASSERT_NE(peers_bot5[0].port(), peers_bot5[1].port());
  ASSERT_NE(peers_bot5[1].port(), peers_bot5[2].port());
  ASSERT_NE(peers_bot5[0].port(), peers_bot5[2].port());

  ASSERT_EQ(peers_bot6[0].endpoint(), "127.0.0.1");
  ASSERT_EQ(peers_bot6[1].endpoint(), "127.0.0.1");
  ASSERT_EQ(peers_bot6[2].endpoint(), "127.0.0.1");
  ASSERT_TRUE(peers_bot6[0].port() == 1350 || peers_bot6[0].port() == 1351 ||
              peers_bot6[0].port() == 1340);
  ASSERT_TRUE(peers_bot6[1].port() == 1350 || peers_bot6[1].port() == 1351 ||
              peers_bot6[1].port() == 1340);
  ASSERT_TRUE(peers_bot6[2].port() == 1350 || peers_bot6[2].port() == 1351 ||
              peers_bot6[2].port() == 1340);
  ASSERT_NE(peers_bot6[0].port(), peers_bot6[1].port());
  ASSERT_NE(peers_bot6[1].port(), peers_bot6[2].port());
  ASSERT_NE(peers_bot6[0].port(), peers_bot6[2].port());

  ASSERT_EQ(peers_bot3[0].endpoint(), "127.0.0.1");
  ASSERT_EQ(peers_bot3[1].endpoint(), "127.0.0.1");
  ASSERT_EQ(peers_bot3[2].endpoint(), "127.0.0.1");
  ASSERT_TRUE(peers_bot3[0].port() == 1350 || peers_bot3[0].port() == 1351 ||
              peers_bot3[0].port() == 1352);
  ASSERT_TRUE(peers_bot3[1].port() == 1350 || peers_bot3[1].port() == 1351 ||
              peers_bot3[1].port() == 1352);
  ASSERT_TRUE(peers_bot3[2].port() == 1350 || peers_bot3[2].port() == 1351 ||
              peers_bot3[2].port() == 1352);
  ASSERT_NE(peers_bot3[0].port(), peers_bot3[1].port());
  ASSERT_NE(peers_bot3[1].port(), peers_bot3[2].port());
  ASSERT_NE(peers_bot3[0].port(), peers_bot3[2].port());
}

TEST(INTEGRATION, terminate_on_bad_version) {
  Listener listener;
  std::shared_ptr<messages::Subscriber> subscriber0;
  {
    Path config_path0("integration_propagation0.json");
    messages::config::Config config0(config_path0);
    auto bot0 = std::make_shared<Bot>(config0);
    Path config_path1("integration_propagation1.json");
    messages::config::Config config1(config_path1);
    auto bot1 = std::make_shared<Bot>(config1);

    std::this_thread::sleep_for(5s);

    subscriber0 = std::make_shared<messages::Subscriber>(bot0->queue());

    subscriber0->subscribe(messages::Type::kConnectionClosed,
                           [&listener](const messages::Header &header,
                                       const messages::Body &body) {
                             listener.handler_deconnection(header, body);
                           });

    auto peers_bot0 = vectorize(bot0->connected_peers());

    ASSERT_TRUE(peers_bot0[0].endpoint() == "127.0.0.1" &&
                peers_bot0[0].port() == 1338);

    auto msg = std::make_shared<messages::Message> ();
    msg->add_bodies()->mutable_get_peers();
    auto header = msg->mutable_header();
    messages::fill_header(header);
    //auto key_pub = header->mutable_key_pub();
    //key_pub->CopyFrom(bot0->get_key_pub());

    header->set_version(neuro::MessageVersion + 100);
    bot0->networking()->send(msg);
  }
  std::this_thread::sleep_for(5s);

  ASSERT_GT(listener.received_deconnection(), 0);
}

TEST(INTEGRATION, keep_max_connections) {
  Path config_path0("integration_keepmax0.json");
  messages::config::Config config0(config_path0);
  auto bot0 = std::make_shared<Bot>(config0);
  std::this_thread::sleep_for(5s);
  Path config_path1("integration_keepmax1.json");
  messages::config::Config config1(config_path1);
  auto bot1 = std::make_shared<Bot>(config1);
  std::this_thread::sleep_for(5s);
  Path config_path2("integration_keepmax2.json");
  messages::config::Config config2(config_path2);
  auto bot2 = std::make_shared<Bot>(config2);
  std::this_thread::sleep_for(5s);

  auto peers_bot0 = vectorize(bot0->connected_peers());
  auto peers_bot1 = vectorize(bot1->connected_peers());
  auto peers_bot2 = vectorize(bot2->connected_peers());

  ASSERT_TRUE(peers_bot0.size() == 1);
  ASSERT_TRUE(peers_bot1.size() == 2);
  ASSERT_TRUE(peers_bot2.size() == 1);

  ASSERT_TRUE(peers_bot0[0].endpoint() == "127.0.0.1" &&
              peers_bot0[0].port() == 1338);

  ASSERT_TRUE(peers_bot1[0].endpoint() == "127.0.0.1" &&
              peers_bot1[0].port() == 1337);

  ASSERT_TRUE(peers_bot2[0].endpoint() == "127.0.0.1" &&
              peers_bot2[0].port() == 1338);
}

TEST(INTEGRATION, block_exchange) {
  ASSERT_TRUE(true);
  // TODO: implement relevant test

  Path config_path0("integration_propagation0.json");
  messages::config::Config config0(config_path0);
  BotTest bot0(config0);
  bot0.add_block();

  std::cout << std::endl << "AVANT" << std::endl << std::endl;
  std::cout << __FILE__ << ":" << __LINE__
            << " Nb of blocks bot 0: " << bot0.nb_blocks() << std::endl;

  std::this_thread::sleep_for(5s);

  Path config_path1("integration_propagation1.json");
  messages::config::Config config1(config_path1);
  BotTest bot1(config1);

  std::cout << std::endl << "AVANT SLEEP" << std::endl << std::endl;
  std::cout << __FILE__ << ":" << __LINE__
            << " Nb of blocks bot 0: " << bot0.nb_blocks() << std::endl;

  std::this_thread::sleep_for(5s);

  std::cout << std::endl << "APRES" << std::endl << std::endl;
  std::cout << __FILE__ << ":" << __LINE__
            << " Nb of blocks bot 0: " << bot0.nb_blocks() << std::endl;

  std::cout << __FILE__ << ":" << __LINE__
            << " Nb of blocks: bot 1: " << bot1.nb_blocks() << std::endl;
}

}  // namespace tests

}  // namespace neuro

