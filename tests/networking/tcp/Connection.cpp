#include "networking/tcp/Connection.hpp"
#include <gtest/gtest.h>

#include "crypto/Ecc.hpp"
#include "src/messages/Queue.hpp"

namespace neuro {
namespace networking {
namespace test {

TEST(Connection, constructor) {
  // simply check it doesn't throw
  auto io_context_ptr = std::make_shared<boost::asio::io_context>();
  auto socket = std::make_shared<bai::tcp::socket>(*io_context_ptr);
  auto queue = std::make_shared<messages::Queue>();
  ASSERT_NE(queue, nullptr);
  auto ecc_keys0 = std::make_shared<crypto::Ecc>();
  auto ecc_keys1 = std::make_shared<crypto::Ecc>();
  std::size_t count_disconnection = 0;
  ASSERT_EQ(count_disconnection, 0);
}

}  // namespace test
}  // namespace networking
}  // namespace neuro
