#include <gtest/gtest.h>

#include "src/common/Buffer.hpp"

namespace neuro {
namespace test {

TEST(Buffer, Hex) {
  {
    bool did_throw = false;
    try {
      const Buffer input("A", Buffer::InputType::HEX);
    } catch (...) {
      did_throw = true;
    }
    EXPECT_TRUE(did_throw);
  }

  {
    const Buffer buf("010A", Buffer::InputType::HEX);
    const Buffer ref{1, 10};
    EXPECT_EQ(buf, ref);
  }

  {
    const Buffer buf("0aF0", Buffer::InputType::HEX);
    const Buffer ref{10, 240};
    EXPECT_EQ(buf, ref);
  }
}

}  // namespace test
}  // namespace neuro
