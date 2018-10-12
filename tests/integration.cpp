#include <gtest/gtest.h>
#include <chrono>
#include <sstream>
#include <thread>
#include "Bot.hpp"
#include "common/logger.hpp"
#include "messages/Subscriber.hpp"

namespace neuro {
using namespace std::chrono_literals;

std::stringstream botconf(const std::string &max_connections,
                          const std::string &listening_port,
                          const std::vector<std::string> &ports) {
  std::stringstream conf;
  conf << "{"
       << "    \"logs\": {"
       << "        \"severity\": \"debug\","
       << "        \"to_stdout\": true"
       << "    },"
       << "    \"networking\" : {"
       << "        \"max_connections\" : " << max_connections << " ,"
       << "        \"tcp\" : {"
       << "            \"port\" : " << listening_port << ",";
  conf << " \"peers\": [";
  if (!ports.empty()) {
    std::size_t i;
    for (i = 0; i < ports.size() - 1; i++) {
      conf << "  {\"endpoint\" : \"127.0.0.1\", \"port\" : " << ports[i]
           << "},";
    }
    conf << "  {\"endpoint\" : \"127.0.0.1\", \"port\" : " << ports[i] << "}";
  }
  conf << "]";
  conf << "        }"
       << "    },"
       << "    \"selection_method\" :\"simple\", "
       << "    \"keep_status\" :\"connected\", "
       << "    \"key_pub_path\" :\"\", "
       << "    \"key_priv_path\" :\"\" "
       << "}";
  return conf;
}

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
    LOG_DEBUG << "It entered the handler_hello";
    _received_hello = true;
  }
  void handler_world(const messages::Header &header,
                     const messages::Body &hello) {
    LOG_DEBUG << "It entered the handler_world";
    _received_world = true;
  }
  void handler_connection0(const messages::Header &header,
                           const messages::Body &body) {
    LOG_DEBUG << "It entered the handler_connection0";
    _received_connection0 = true;
  }
  void handler_connection1(const messages::Header &header,
                           const messages::Body &body) {
    LOG_DEBUG << "It entered the handler_connection1";
    _received_connection1 = true;
  }

  bool validated() {
    std::cout << std::boolalpha << "validation " << _received_connection0 << " "
              << _received_connection1 << " " << _received_hello << " "
              << _received_world;
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

  //   bots[1]->keep_max_connections();
  //   bots[2]->keep_max_connections();
  //   std::this_thread::sleep_for(100ms);
  //   bots[0]->keep_max_connections();
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

TEST(COIN, simple_interaction) {
  auto conf0 = botconf("3", "1337", {});
  auto conf1 = botconf("3", "1338", {"1337", "1339"});
  auto conf2 = botconf("3", "1339", {"1337", "1338"});

  // { Bot bot(conf2); }
  // return true;

  Listener listener;
  std::vector<std::shared_ptr<Bot>> bots;
  bots.emplace_back(std::make_shared<Bot>(conf0));
  bots.emplace_back(std::make_shared<Bot>(conf1));

  messages::Subscriber subscriber0(bots[0]->queue());
  messages::Subscriber subscriber1(bots[1]->queue());

  int bot_id = 0;

  std::cout << conf1.str() << std::endl;

  for (const auto &bot : bots) {
    std::cout << bot_id++ << " bot_id " << bot.get() << std::endl;
  }

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
  bots[0]->keep_max_connections();
  bots[1]->keep_max_connections();

  std::this_thread::sleep_for(500ms);

  for (const auto &bot : bots) {
    bot->status();
  }

  std::this_thread::sleep_for(15s);
  // auto message = std::make_shared<messages::Message>();
  // message->add_bodies()->mutable_hello();
  // bots[0]->networking()->send(message, networking::ProtocolType::PROTOBUF2);

  std::this_thread::sleep_for(500ms);
  ASSERT_TRUE(listener.validated());

  std::this_thread::sleep_for(500ms);
  LOG_DEBUG << this << " About to go out";
}

}  // namespace neuro
