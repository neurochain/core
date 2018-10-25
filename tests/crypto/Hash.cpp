#include <gtest/gtest.h>

#include "src/crypto/Hash.hpp"
#include "common/logger.hpp"

namespace neuro {
namespace crypto {
namespace test {

TEST(Hash, SHA3_256) {
  {
    const Buffer input("Hello world!");
    const auto h = hash_sha3_256(input);
    const Buffer ref(
        "d6ea8f9a1f22e1298e5a9506bd066f23cc56001f5d36582344a628649df53ae8",
        Buffer::InputType::HEX);
    ASSERT_EQ(h, ref);
  }
  {
    const Buffer input("");
    const auto h = hash_sha3_256(input);
    const Buffer ref(
        "a7ffc6f8bf1ed76651c14756a061d662f580ff4de43b49fa82d80a4b80f8434a",
        Buffer::InputType::HEX);
    ASSERT_EQ(h, ref);
  }
}

}  // namespace test
}  // namespace crypto
}  // namespace neuro
