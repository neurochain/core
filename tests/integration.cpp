#include <gtest/gtest.h>
#include <chrono>
#include <sstream>
#include <thread>
#include "Bot.hpp"
#include "common/logger.hpp"
#include "messages/Subscriber.hpp"

namespace neuro {
using namespace std::chrono_literals;

class Listener {
 private:
  bool _received_connection0{false};
  bool _received_connection1{false};
  bool _received_hello{false};
  bool _received_world{false};

 public:
  Listener() {}

  void handler_hello(const messages::Header &header,
                     const messages::Body &hello) {
    LOG_DEBUG << __FILE__ << ": It entered the handler_hello !!";
    _received_hello = true;
  }
  void handler_world(const messages::Header &header,
                     const messages::Body &hello) {
    LOG_DEBUG << __FILE__ << ": It entered the handler_world !!";
    _received_world = true;
  }
  void handler_connection0(const messages::Header &header,
                           const messages::Body &body) {
    LOG_DEBUG << __FILE__ << ": It entered the handler_connection0 !!";
    _received_connection0 = true;
  }
  void handler_connection1(const messages::Header &header,
                           const messages::Body &body) {
    LOG_DEBUG << __FILE__ << ": It entered the handler_connection1 !!";
    _received_connection1 = true;
  }

  bool validated() {
    return _received_connection0 && _received_connection1 && _received_hello &&
           _received_world;
  }

  ~Listener() {
    LOG_DEBUG << "Entered the destructor";
    // EXPECT_TRUE(_received_connection0);
    // EXPECT_TRUE(_received_connection1);
    // EXPECT_TRUE(_received_hello);
    // EXPECT_TRUE(_received_world);
  }
};

class Integration : public ::testing::Test {
 public:
  Integration() {}

  // bool test_reachmax() {
  //   auto conf0 = botconf("2", "1337", {"1338", "1339"});
  //   auto conf1 = botconf("1", "1338", {});
  //   auto conf2 = botconf("1", "1339", {});
  //   auto conf3 = botconf("1", "1340", {"1337"});

  //   std::vector<std::shared_ptr<Bot>> bots{
  //       std::make_shared<Bot>(conf0),
  //       std::make_shared<Bot>(conf1),
  //       std::make_shared<Bot>(conf2),
  //       std::make_shared<Bot>(conf3)};

  //   bot1->keep_max_connections();
  //   bots[2]->keep_max_connections();
  //   std::this_thread::sleep_for(100ms);
  //   bot0->keep_max_connections();
  //   std::this_thread::sleep_for(200ms);
  //   bots[3]->keep_max_connections();
  //   std::this_thread::sleep_for(100ms);

  //   for (auto &bot : bots) {
  //     bot->networking()->unsubscribe(bot.get());
  //   }

  //   std::this_thread::sleep_for(100ms);

  //   std::vector<Bot::Status> status;
  //   for (const auto &bot : bots) {
  //     LOG_DEBUG << *bot;
  //     status.emplace_back(bot->status());
  //   }

  //   // validate that bot[3] has connected:0, peers:3, max_connections:1
  //   bool all_good =
  //       status[0].connected_peers == 2 && status[1].connected_peers == 1 &&
  //       status[2].connected_peers == 1 && status[3].connected_peers == 0;

  //   return all_good;
  // }
};

TEST(INTEGRATION, simple_interaction) {
  Listener listener;
  std::vector<std::shared_ptr<Bot>> bots;
  auto bot0 = std::make_shared<Bot>("bot0.json");
  auto bot1 = std::make_shared<Bot>("bot1.json");

  messages::Subscriber subscriber0(bot0->queue());
  messages::Subscriber subscriber1(bot1->queue());
  std::cout << __LINE__ << std::endl;

  int bot_id = 0;
  std::cout << __LINE__ << std::endl;

  for (const auto &bot : bots) {
    std::cout << bot_id++ << " bot_id " << bot.get() << std::endl;
  }
  std::cout << __LINE__ << std::endl;

  subscriber0.subscribe(
      messages::Type::kHello,
      [&listener](const messages::Header &header, const messages::Body &body) {
        listener.handler_hello(header, body);
      });
  subscriber1.subscribe(
      messages::Type::kWorld,
      [&listener](const messages::Header &header, const messages::Body &body) {
        listener.handler_world(header, body);
      });
  subscriber0.subscribe(
      messages::Type::kConnectionReady,
      [&listener](const messages::Header &header, const messages::Body &body) {
        listener.handler_connection0(header, body);
      });
  subscriber1.subscribe(
      messages::Type::kConnectionReady,
      [&listener](const messages::Header &header, const messages::Body &body) {
        listener.handler_connection1(header, body);
      });

  std::this_thread::sleep_for(1s);
  std::cout << __LINE__ << std::endl;
  bot0->keep_max_connections();
  bot1->keep_max_connections();
  std::cout << __LINE__ << std::endl;
  std::this_thread::sleep_for(500ms);
  std::cout << __LINE__ << std::endl;

  for (const auto &bot : bots) {
    bot->status();
  }
  std::cout << __LINE__ << std::endl;

  std::this_thread::sleep_for(3s);
  // auto message = std::make_shared<messages::Message>();
  // message->add_bodies()->mutable_hello();
  // bot0->networking()->send(message, networking::ProtocolType::PROTOBUF2);

  std::this_thread::sleep_for(500ms);
  ASSERT_TRUE(listener.validated());

  LOG_DEBUG << " About to go out";
}

TEST(INTEGRATION, neighbors_propagation) {
  auto bot0 = std::make_shared<Bot>("integration_propagation0.json");
  auto bot1 = std::make_shared<Bot>("integration_propagation1.json");
  auto bot2 = std::make_shared<Bot>("integration_propagation2.json");

  std::this_thread::sleep_for(100ms);
  bot0->keep_max_connections();
  std::this_thread::sleep_for(500ms);
  bot1->keep_max_connections();
  std::this_thread::sleep_for(500ms);
  bot2->keep_max_connections();
  std::this_thread::sleep_for(2500ms);
  auto peers_bot0 = bot0->connected_peers();
  auto peers_bot1 = bot1->connected_peers();
  auto peers_bot2 = bot2->connected_peers();

  std::cout << __FILE__ << ":" << __LINE__ << " " << this
            << " peers_bot0::size: " << peers_bot0.size() << std::endl;
  std::cout << __FILE__ << ":" << __LINE__ << " " << this
            << " peers_bot1::size: " << peers_bot1.size() << std::endl;
  std::cout << __FILE__ << ":" << __LINE__ << " " << this
            << " peers_bot2::size: " << peers_bot2.size() << std::endl;

  ASSERT_TRUE(peers_bot0.size() == peers_bot1.size() &&
              peers_bot1.size() == peers_bot2.size() && peers_bot2.size() == 2);

  ASSERT_TRUE(peers_bot0[0].endpoint() == "127.0.0.1" &&
              peers_bot0[0].port() == 1338);
  ASSERT_TRUE(peers_bot0[1].endpoint() == "127.0.0.1" &&
              peers_bot0[1].port() == 1339);

  ASSERT_TRUE(peers_bot1[0].endpoint() == "127.0.0.1" &&
              peers_bot1[0].port() == 1337);
  ASSERT_TRUE(peers_bot1[1].endpoint() == "127.0.0.1" &&
              peers_bot1[1].port() == 1339);

  ASSERT_TRUE(peers_bot2[0].endpoint() == "127.0.0.1" &&
              peers_bot2[0].port() == 1337);
  ASSERT_TRUE(peers_bot2[1].endpoint() == "127.0.0.1" &&
              peers_bot2[1].port() == 1338);
}

TEST(INTEGRATION, block_exchange) {
  ASSERT_TRUE(true);
  // iniciar el bot
  auto bot0 = std::make_shared<Bot>("bot0.json");
  // dump el mensaje del block0 para validar
}

}  // namespace neuro
