#include "messages/Address.hpp"
#include <cryptopp/hex.h>
#include <gtest/gtest.h>

namespace neuro {
namespace messages {
namespace test {

TEST(Address, encode_base58) {
  // There is no 0 in base58
  ASSERT_EQ(encode_base58(0, ""), "1");
  ASSERT_EQ(encode_base58(1, ""), "2");
  ASSERT_EQ(encode_base58(57, ""), "z");
  ASSERT_EQ(encode_base58(58, ""), "21");
  ASSERT_EQ(encode_base58(2 * 58, ""), "31");
  ASSERT_EQ(encode_base58(57 * 58 + 57, ""), "zz");
  ASSERT_EQ(encode_base58(58 * 58, ""), "211");
  ASSERT_EQ(encode_base58(0, "TITI"), "TITI1");
}

}  // namespace test
}  // namespace messages
}  // namespace neuro
