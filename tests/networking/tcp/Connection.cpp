#include "networking/tcp/Connection.hpp"
#include <gtest/gtest.h>

#include "src/messages/Queue.hpp"

namespace neuro {
namespace networking {
namespace test {

TEST(Connection, constructor) {
  // simply check it doesn't throw
  auto io_context_ptr = std::make_shared<boost::asio::io_context>();
  auto socket = std::make_shared<bai::tcp::socket>(*io_context_ptr);
  auto queue = messages::Queue{};
  messages::Peer peer;
  tcp::Connection connection_0(0, &queue, socket, &peer);
  tcp::Connection connection_1(345, &queue, socket, &peer);
}

}  // namespace test
}  // namespace networking
}  // namespace neuro
