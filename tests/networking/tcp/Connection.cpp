#include <gtest/gtest.h>

#include "networking/tcp/Connection.hpp"

namespace neuro {
namespace networking {
namespace test {

TEST(Connection, constructor) {
  // simply check it doesn't throw
  auto io_context_ptr = std::make_shared<boost::asio::io_context>();
  LOG_TRACE << std::endl;
  auto socket = std::make_shared<bai::tcp::socket>(*io_context_ptr);
  LOG_TRACE << std::endl;
  auto queue = messages::Queue{};
  LOG_TRACE << std::endl;
  auto conf = messages::config::Config{Path("./bot2.json")};
  LOG_TRACE << std::endl;
  auto peer = std::make_shared<messages::Peer>(conf.networking());
  LOG_TRACE << std::endl;
  tcp::Connection connection_0(0, &queue, socket, peer);
  LOG_TRACE << std::endl;
  tcp::Connection connection_1(345, &queue, socket, peer);
  LOG_TRACE << std::endl;
}

}  // namespace test
}  // namespace networking
}  // namespace neuro
