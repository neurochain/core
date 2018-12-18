// #include <gtest/gtest.h>
// #include <chrono>
// #include <sstream>
// #include <thread>
// #include "Bot.hpp"
// #include "common/logger.hpp"
// #include "common/types.hpp"
// #include "consensus/PiiConsensus.hpp"
// #include "crypto/Sign.hpp"
// #include "ledger/LedgerMongodb.hpp"
// #include "messages/Subscriber.hpp"
// #include "messages/config/Config.hpp"
// #include "tooling/genblock.hpp"

// namespace neuro {

// namespace tests {

// using namespace std::chrono_literals;

// class Listener {
//  private:
//   std::size_t _received_connection0{0};
//   std::size_t _received_connection1{0};
//   std::size_t _received_hello{0};
//   std::size_t _received_world{0};
//   std::size_t _received_deconnection{0};

//  public:
//   Listener() {}

//   void handler_hello(const messages::Header &header,
//                      const messages::Body &hello) {
//     LOG_DEBUG << __FILE__ << ": It entered the handler_hello !!";
//     ++_received_hello;
//   }
//   void handler_world(const messages::Header &header,
//                      const messages::Body &hello) {
//     LOG_DEBUG << __FILE__ << ": It entered the handler_world !!";
//     ++_received_world;
//   }
//   void handler_connection0(const messages::Header &header,
//                            const messages::Body &body) {
//     LOG_DEBUG << __FILE__ << ": It entered the handler_connection0 !!";
//     ++_received_connection0;
//   }
//   void handler_connection1(const messages::Header &header,
//                            const messages::Body &body) {
//     LOG_DEBUG << __FILE__ << ": It entered the handler_connection1 !!";
//     ++_received_connection1;
//   }

//   void handler_deconnection(const messages::Header &header,
//                             const messages::Body &body) {
//     ++_received_deconnection;
//   }

//   std::size_t received_connection0() const { return _received_connection0; };
//   std::size_t received_connection1() const { return _received_connection1; };
//   std::size_t received_hello() const { return _received_hello; };
//   std::size_t received_world() const { return _received_world; };
//   std::size_t received_deconnection() const { return _received_deconnection; };
// };

// class BotTest {
//  private:
//   neuro::Bot _bot;

//  public:
//   BotTest(const std::string &configpath) : _bot(configpath) {}

//   int nb_blocks() { return _bot._ledger->total_nb_blocks(); }

//   void add_block() {
//     messages::Block new_block;
//     neuro::tooling::genblock::genblock_from_last_db_block(new_block,
//                                                           _bot._ledger, 1, 0);
//     _bot._ledger->push_block(new_block);
//   }
// };

// TEST(INTEGRATION, simple_interaction) {
//   Listener listener;
//   auto bot0 = std::make_shared<Bot>("bot0.json");
//   auto bot1 = std::make_shared<Bot>("bot1.json");
//   auto bot2 = std::make_shared<Bot>("bot2.json");

//   messages::Subscriber subscriber0(bot0->queue());
//   messages::Subscriber subscriber1(bot1->queue());
//   messages::Subscriber subscriber2(bot2->queue());
//   std::cout << __LINE__ << std::endl;

//   subscriber0.subscribe(
//       messages::Type::kHello,
//       [&listener](const messages::Header &header, const messages::Body &body)
//       {
//         listener.handler_hello(header, body);
//       });
//   subscriber1.subscribe(
//       messages::Type::kHello,
//       [&listener](const messages::Header &header, const messages::Body &body)
//       {
//         listener.handler_hello(header, body);
//       });
//   subscriber2.subscribe(
//       messages::Type::kHello,
//       [&listener](const messages::Header &header, const messages::Body &body)
//       {
//         listener.handler_hello(header, body);
//       });
//   subscriber0.subscribe(
//       messages::Type::kWorld,
//       [&listener](const messages::Header &header, const messages::Body &body)
//       {
//         listener.handler_world(header, body);
//       });
//   subscriber1.subscribe(
//       messages::Type::kWorld,
//       [&listener](const messages::Header &header, const messages::Body &body)
//       {
//         listener.handler_world(header, body);
//       });
//   subscriber2.subscribe(
//       messages::Type::kWorld,
//       [&listener](const messages::Header &header, const messages::Body &body)
//       {
//         listener.handler_world(header, body);
//       });
//   subscriber0.subscribe(
//       messages::Type::kConnectionReady,
//       [&listener](const messages::Header &header, const messages::Body &body)
//       {
//         listener.handler_connection0(header, body);
//       });
//   subscriber1.subscribe(
//       messages::Type::kConnectionReady,
//       [&listener](const messages::Header &header, const messages::Body &body)
//       {
//         listener.handler_connection1(header, body);
//       });

//   std::this_thread::sleep_for(1s);
//   std::cout << __LINE__ << std::endl;
//   bot0->keep_max_connections();
//   bot1->keep_max_connections();
//   std::cout << __LINE__ << std::endl;
//   std::this_thread::sleep_for(500ms);
//   std::cout << __LINE__ << std::endl;

//   std::this_thread::sleep_for(3s);

//   ASSERT_GT(listener.received_connection0() +
//   listener.received_connection1(), 0);
//   ASSERT_GT(listener.received_connection0(), 0);
//   ASSERT_GT(listener.received_connection1(), 0);
//   ASSERT_GT(listener.received_hello(), 0);
//   ASSERT_GT(listener.received_world(), 0);
//   ASSERT_EQ(listener.received_deconnection(), 0);
// }

// TEST(INTEGRATION, neighbors_propagation) {
//   auto bot0 = std::make_shared<Bot>(
//       messages::config::Config("integration_propagation0.json"));
//   auto bot1 = std::make_shared<Bot>(
//       messages::config::Config("integration_propagation1.json"));
//   auto bot2 = std::make_shared<Bot>(
//       messages::config::Config("integration_propagation2.json"));

//   std::this_thread::sleep_for(100ms);
//   bot0->keep_max_connections();
//   std::this_thread::sleep_for(500ms);
//   bot1->keep_max_connections();
//   std::this_thread::sleep_for(500ms);
//   bot2->keep_max_connections();
//   std::this_thread::sleep_for(2500ms);
//   auto peers_bot0 = bot0->connected_peers();
//   auto peers_bot1 = bot1->connected_peers();
//   auto peers_bot2 = bot2->connected_peers();

//   ASSERT_EQ(peers_bot0.size(), peers_bot1.size());
//   ASSERT_EQ(peers_bot1.size(), peers_bot2.size());
//   ASSERT_EQ(peers_bot2.size(), 2);

//   ASSERT_EQ(peers_bot0[0].endpoint(), "127.0.0.1");
//   ASSERT_EQ(peers_bot0[1].endpoint(), "127.0.0.1");
//   ASSERT_TRUE(peers_bot0[0].port() == 1338 || peers_bot0[0].port() == 1339);
//   ASSERT_TRUE(peers_bot0[1].port() == 1338 || peers_bot0[1].port() == 1339);
//   ASSERT_NE(peers_bot0[1].port(), peers_bot0[1].port());

//   ASSERT_EQ(peers_bot1[0].endpoint(), "127.0.0.1");
//   ASSERT_EQ(peers_bot1[1].endpoint(), "127.0.0.1");
//   ASSERT_TRUE(peers_bot1[0].port() == 1337 || peers_bot1[0].port() == 1339);
//   ASSERT_TRUE(peers_bot1[1].port() == 1337 || peers_bot1[1].port() == 1339);
//   ASSERT_NE(peers_bot1[1].port(), peers_bot1[1].port());

//   ASSERT_EQ(peers_bot2[0].endpoint(), "127.0.0.1");
//   ASSERT_EQ(peers_bot2[1].endpoint(), "127.0.0.1");
//   ASSERT_TRUE(peers_bot2[0].port() == 1337 || peers_bot2[0].port() == 1338);
//   ASSERT_TRUE(peers_bot2[1].port() == 1337 || peers_bot2[1].port() == 1338);
//   ASSERT_NE(peers_bot2[1].port(), peers_bot2[1].port());
// }

// TEST(INTEGRATION, neighbors_update) {
//   auto bot0 = std::make_shared<Bot>("integration_update0.json");
//   std::this_thread::sleep_for(1s);
//   auto bot1 = std::make_shared<Bot>("integration_update1.json");
//   std::this_thread::sleep_for(1s);
//   auto bot2 = std::make_shared<Bot>("integration_update2.json");
//   std::this_thread::sleep_for(1s);

//   std::this_thread::sleep_for(15s);

//   auto peers_bot0 = bot0->connected_peers();
//   auto peers_bot1 = bot1->connected_peers();
//   auto peers_bot2 = bot2->connected_peers();

//   ASSERT_EQ(peers_bot0.size(), peers_bot1.size());
//   ASSERT_EQ(peers_bot1.size(), peers_bot2.size());
//   ASSERT_EQ(peers_bot2.size(), 2);

//   ASSERT_EQ(peers_bot0[0].endpoint(), "127.0.0.1");
//   ASSERT_EQ(peers_bot0[1].endpoint(), "127.0.0.1");
//   ASSERT_TRUE(peers_bot0[0].port() == 1338 || peers_bot0[0].port() == 1339);
//   ASSERT_TRUE(peers_bot0[1].port() == 1338 || peers_bot0[1].port() == 1339);
//   ASSERT_NE(peers_bot0[0].port(), peers_bot0[1].port());

//   ASSERT_EQ(peers_bot1[0].endpoint(), "127.0.0.1");
//   ASSERT_EQ(peers_bot1[1].endpoint(), "127.0.0.1");
//   ASSERT_TRUE(peers_bot1[0].port() == 1337 || peers_bot1[0].port() == 1339);
//   ASSERT_TRUE(peers_bot1[1].port() == 1337 || peers_bot1[1].port() == 1339);
//   ASSERT_NE(peers_bot1[0].port(), peers_bot1[1].port());

//   ASSERT_EQ(peers_bot2[0].endpoint(), "127.0.0.1");
//   ASSERT_EQ(peers_bot2[1].endpoint(), "127.0.0.1");
//   ASSERT_TRUE(peers_bot2[0].port() == 1337 || peers_bot2[0].port() == 1338);
//   ASSERT_TRUE(peers_bot2[1].port() == 1337 || peers_bot2[1].port() == 1338);
//   ASSERT_NE(peers_bot2[0].port(), peers_bot2[1].port());
// }

// TEST(INTEGRATION, terminate_on_bad_version) {
//   auto bot0 = std::make_shared<neuro::Bot>("integration_propagation0.json");
//   auto bot1 = std::make_shared<neuro::Bot>("integration_propagation1.json");

//   std::this_thread::sleep_for(500ms);

//   Listener listener;
//   messages::Subscriber subscriber0(bot0->queue());

//   subscriber0.subscribe(
//       messages::Type::kConnectionClosed,
//       [&listener](const messages::Header &header, const messages::Body &body)
//       {
//         listener.handler_deconnection(header, body);
//       });

//   auto peers_bot0 = bot0->connected_peers();

//   ASSERT_TRUE(peers_bot0[0].endpoint() == "127.0.0.1" &&
//               peers_bot0[0].port() == 1338);

//   auto msg = std::make_shared<messages::Message>();
//   msg->add_bodies()->mutable_get_peers();
//   auto header = msg->mutable_header();
//   messages::fill_header(header);
//   header->set_version(neuro::MessageVersion + 100);
//   bot0->networking()->send(msg, neuro::networking::ProtocolType::PROTOBUF2);

//   std::this_thread::sleep_for(500ms);

//   ASSERT_GT(listener.received_deconnection(), 0);
// }

// TEST(INTEGRATION, keep_max_connections) {
//   auto bot0 = std::make_shared<neuro::Bot>("integration_keepmax0.json");
//   std::this_thread::sleep_for(750ms);
//   auto bot1 = std::make_shared<neuro::Bot>("integration_keepmax1.json");
//   std::this_thread::sleep_for(750ms);
//   auto bot2 = std::make_shared<neuro::Bot>("integration_keepmax2.json");
//   std::this_thread::sleep_for(750ms);

//   auto peers_bot0 = bot0->connected_peers();
//   auto peers_bot1 = bot1->connected_peers();
//   auto peers_bot2 = bot2->connected_peers();

//   ASSERT_TRUE(peers_bot0.size() == 1);
//   ASSERT_TRUE(peers_bot1.size() == 2);
//   ASSERT_TRUE(peers_bot2.size() == 1);

//   ASSERT_TRUE(peers_bot0[0].endpoint() == "127.0.0.1" &&
//               peers_bot0[0].port() == 1338);

//   ASSERT_TRUE(peers_bot1[0].endpoint() == "127.0.0.1" &&
//               peers_bot1[0].port() == 1337);

//   ASSERT_TRUE(peers_bot2[0].endpoint() == "127.0.0.1" &&
//               peers_bot2[0].port() == 1338);
// }

// TEST(INTEGRATION, block_exchange) {
//   ASSERT_TRUE(true);

//   BotTest bot0("integration_propagation0.json");
//   bot0.add_block();
//   BotTest bot1("integration_propagation1.json");

//   // std::this_thread::sleep_for(35s);

//   std::cout << __FILE__ << ":" << __LINE__
//             << " Nb of blocks: " << bot0.nb_blocks() << std::endl;

//   std::cout << __FILE__ << ":" << __LINE__
//             << " Nb of blocks: " << bot1.nb_blocks() << std::endl;
// }

// }  // namespace tests

// }  // namespace neuro
