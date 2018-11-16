#include "networking/tcp/Connection.hpp"
#include <gtest/gtest.h>

#include "src/messages/Queue.hpp"

namespace neuro {
namespace networking {
namespace test {

TEST(Connection, constructor) {
  // simply check it doesn throw
  auto io_service_ptr = std::make_shared<boost::asio::io_service>();
  auto socket = std::make_shared<bai::tcp::socket>(*io_service_ptr);
  auto queue = std::make_shared<messages::Queue>();
  ASSERT_NE(queue, nullptr);
  auto peer = std::make_shared<messages::Peer>();
  tcp::Connection connection_0(0, 0, io_service_ptr, queue, socket, peer);
  tcp::Connection connection_1(345, 6456, io_service_ptr, queue, socket, peer);
  // check it throws
  try {
    tcp::Connection connection_2(0, 0, io_service_ptr, nullptr, socket, peer);
    ASSERT_FALSE(true);
  } catch (...) {
  }
  try {
    tcp::Connection connection_3(0, 0, io_service_ptr, queue, nullptr, peer);
    ASSERT_FALSE(true);
  } catch (...) {
  }
  try {
    tcp::Connection connection_4(0, 0, io_service_ptr, queue, socket, nullptr);
    ASSERT_FALSE(true);
  } catch (...) {
  }
}

}  // namespace test
}  // namespace networking
}  // namespace neuro
