#include "Bot.hpp"
#include "messages/Subscriber.hpp"
#include "common/logger.hpp"
#include <chrono>
#include <gtest/gtest.h>
#include <sstream>
#include <thread>

namespace neuro {
using namespace std::chrono_literals;

std::stringstream botconf(const std::string &max_connections,
                          const std::string &listening_port) {
  std::stringstream conf;
  conf << "{"
       << "    \"logs\": {"
       << "        \"severity\": \"debug\","
       << "        \"to_stdout\": true"
       << "    },"
       << "    \"networking\" : {"
       << "        \"max_connections\" : " << max_connections << " ,"
       << "        \"tcp\" : {"
       << "            \"listen_ports\" : [ " << listening_port << " ]"
       << "        }"
       << "    },"
       << "    \"selection_method\" :\"SIMPLE\" "
       << "}";
  return conf;
}

std::stringstream peersconf(const std::vector<std::string> &ports) {
  std::stringstream conf;
  conf << "{ \"peers\": [";
  if (!ports.empty()) {
    std::size_t i;
    for (i = 0; i < ports.size() - 1; i += 1) {
      conf << "  {\"ip\" : \"127.0.0.1\", \"port\" : " << ports[i] << "},";
    }
    conf << "  {\"ip\" : \"127.0.0.1\", \"port\" : " << ports[i] << "}";
  }
  conf << "]}";
  return conf;
}

class Listener {
private:
  bool _received_connection0{false};
  bool _received_connection1{false};
  bool _received_hello{false};
  bool _received_world{false};
  messages::Subscriber _subscriber;

public:
  Listener() : _subscriber(std::make_shared<messages::Queue>()) {}

  void handler_hello(std::shared_ptr<const messages::Header> header,
                     std::shared_ptr<const messages::Body> hello) {
    LOG_DEBUG << "It entered the handler_hello";
    _received_hello = true;
  }
  void handler_world(std::shared_ptr<const messages::Header> header,
                     std::shared_ptr<const messages::Body> hello) {

    LOG_DEBUG << "It entered the handler_world";
    _received_world = true;
  }
  void handler_connection0(std::shared_ptr<const messages::Header> header,
                           std::shared_ptr<const messages::Body> body) {

    LOG_DEBUG << "It entered the handler_connection0";
    _received_connection0 = true;
  }
  void handler_connection1(std::shared_ptr<const messages::Header> header,
                           std::shared_ptr<const messages::Body> body) {

    LOG_DEBUG << "It entered the handler_connection1";
    _received_connection1 = true;
  }

  bool validated() {
    return _received_connection0 && _received_connection1 && _received_hello &&
           _received_world;
  }

  ~Listener() {
    LOG_DEBUG << "Entered the destructor";
    EXPECT_TRUE(_received_connection0);
    EXPECT_TRUE(_received_connection1);
    EXPECT_TRUE(_received_hello);
    EXPECT_TRUE(_received_world);
  }
};

class Integration : public ::testing::Test {
public:
  Integration() {}

  bool test_reachmax() {
    auto conf0 = botconf("2", "1337");
    auto conf1 = botconf("1", "1338");
    auto conf2 = botconf("1", "1339");
    auto conf3 = botconf("1", "1340");

    auto peers0 = peersconf({"1338", "1339"});
    auto peers1 = peersconf({});
    auto peers2 = peersconf({});
    auto peers3 = peersconf({"1337"});

    std::vector<std::shared_ptr<Bot>> bots{
        std::make_shared<Bot>(conf0, peers0),
        std::make_shared<Bot>(conf1, peers1),
        std::make_shared<Bot>(conf2, peers2),
        std::make_shared<Bot>(conf3, peers3)};

    bots[1]->keep_max_connections();
    bots[2]->keep_max_connections();
    std::this_thread::sleep_for(100ms);
    bots[0]->keep_max_connections();
    std::this_thread::sleep_for(200ms);
    bots[3]->keep_max_connections();
    std::this_thread::sleep_for(100ms);

    for (auto &bot : bots) {
      bot->networking()->unsubscribe(bot.get());
    }

    std::this_thread::sleep_for(100ms);

    std::vector<Bot::Status> status;
    for (const auto &bot : bots) {
      LOG_DEBUG << *bot;
      status.emplace_back(bot->status());
    }

    // validate that bot[3] has connected:0, peers:3, max_connections:1
    bool all_good =
        status[0].connected_peers == 2 && status[1].connected_peers == 1 &&
        status[2].connected_peers == 1 && status[3].connected_peers == 0;

    return all_good;
  }

  bool test_interaction() {
    auto conf0 = botconf("3", "1337");
    auto peers0 = peersconf({});

    auto conf1 = botconf("3", "1338");
    auto peers1 = peersconf({"1337", "1339"});

    auto conf2 = botconf("3", "1339");
    auto peers2 = peersconf({"1337", "1338"});

    { Bot bot(conf2, peers2); }
    Listener listener;
    std::vector<std::shared_ptr<Bot>> bots;
    bots.emplace_back(std::make_shared<Bot>(conf0, peers0));
    bots.emplace_back(std::make_shared<Bot>(conf1, peers1));

    bots[1]->networking()->subscribe(
        &listener, messages::Type::HELLO,
        [&listener](std::shared_ptr<const messages::Header> header,
                    std::shared_ptr<const messages::Body> body) {
          listener.handler_hello(header, body);
        });
    bots[0]->networking()->subscribe(
        &listener, messages::Type::WORLD,
        [&listener](std::shared_ptr<const messages::Header> header,
                    std::shared_ptr<const messages::Body> body) {
          listener.handler_world(header, body);
        });
    bots[0]->networking()->subscribe(
        &listener, messages::Type::CONNECTION,
        [&listener](std::shared_ptr<const messages::Header> header,
                    std::shared_ptr<const messages::Body> body) {
          listener.handler_connection0(header, body);
        });
    bots[1]->networking()->subscribe(
        &listener, messages::Type::CONNECTION,
        [&listener](std::shared_ptr<const messages::Header> header,
                    std::shared_ptr<const messages::Body> body) {
          listener.handler_connection1(header, body);
        });

    bots[0]->keep_max_connections();
    bots[1]->keep_max_connections();

    std::this_thread::sleep_for(100ms);

    for (const auto &bot : bots) {
      bot->status();
    }

    std::this_thread::sleep_for(1s);
    auto message = std::make_shared<messages::Message>();
    message->insert(std::make_shared<messages::Hello>(1337));
    bots[0]->networking()->send(message);

    std::this_thread::sleep_for(250ms);
    auto res = listener.validated();

    for (auto &bot : bots) {
      bot->networking()->unsubscribe(&listener);
      bot->networking()->unsubscribe(bot.get());
    }
    listener.networking()->unsubscribe(&listener);
    return res;
  }
};

TEST_F(Integration, simple_interaction) { ASSERT_TRUE(test_interaction()); }

TEST_F(Integration, max_connections) { ASSERT_TRUE(test_reachmax()); }

} // namespace neuro
