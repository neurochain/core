#include <gtest/gtest.h>
#include <sstream>

#include "src/common/Buffer.hpp"

namespace neuro {
namespace test {

TEST(Buffer, Ctors) {
  std::size_t step(0);
  try {
    Buffer tested_buffer;
    std::stringstream output;
    output << tested_buffer;
    ++step;
  } catch (...) {
  }
  EXPECT_EQ(step, 1);
  try {
    Buffer copied_buffer;
    Buffer tested_buffer(copied_buffer);
    std::stringstream output;
    output << tested_buffer;
    ++step;
  } catch (...) {
  }
  EXPECT_EQ(step, 2);
  try {
    Buffer tested_buffer((Buffer()));
    std::stringstream output;
    output << tested_buffer;
    ++step;
  } catch (...) {
  }
  EXPECT_EQ(step, 3);
  try {
    Buffer tested_buffer(4, (uint8_t)'A');
    std::stringstream output;
    output << tested_buffer;
    std::string expected_str("41414141");
    ASSERT_EQ(output.str(), expected_str);
    ++step;
  } catch (...) {
  }
  EXPECT_EQ(step, 4);
  try {
    uint8_t raw_data[6] = {1, 2, 3, 5, 77, 255};
    Buffer tested_buffer(raw_data, 6);
    std::stringstream output;
    output << tested_buffer;
    std::string expected_str("010203054dff");
    ASSERT_EQ(output.str(), expected_str);
    ++step;
  } catch (...) {
  }
  EXPECT_EQ(step, 5);
  try {
    std::string initial_str("0102030577FF");
    Buffer tested_buffer(initial_str);
    std::stringstream output;
    output << tested_buffer;
    std::string expected_str("303130323033303537374646");
    ASSERT_EQ(output.str(), expected_str);
    ++step;
  } catch (...) {
  }
  EXPECT_EQ(step, 6);
  try {
    std::string initial_str("0102030507FF");
    Buffer tested_buffer(initial_str, Buffer::InputType::HEX);
    std::stringstream output;
    output << tested_buffer;
    std::string expected_str("0102030507ff");
    ASSERT_EQ(output.str(), expected_str);
    ++step;
  } catch (...) {
  }
  EXPECT_EQ(step, 7);
  try {
    Buffer tested_buffer({1, 2, 3, 5, 77, 255});
    std::stringstream output;
    output << tested_buffer;
    std::string expected_str("010203054dff");
    ASSERT_EQ(output.str(), expected_str);
    ++step;
  } catch (...) {
  }
  EXPECT_EQ(step, 8);
}

TEST(Buffer, save) {
  Buffer tested_buffer({1, 2, 3, 5, 77, 255});
  bool did_throw(false);
  try {
    tested_buffer.save("/tmp/test_buffer_save.txt");
  } catch (...) {
    did_throw = true;
  }
  ASSERT_FALSE(did_throw);
  try {
    tested_buffer.save("/tmp/@@@.txt");
  } catch (...) {
    did_throw = true;
  }
  // todo: check what is expected
  ASSERT_FALSE(did_throw);
}

TEST(Buffer, str) {
  // todo: check what is expected
  Buffer tested_buffer({1, 2, 3, 5, 77, 255});
  std::string expected_str("010203054dff");
  tested_buffer.str();
  // ASSERT_EQ(expected_str, tested_buffer.str());
}

TEST(Buffer, cmp) {
  Buffer tested_buffer({255, 128, 32});
  Buffer identical_buffer({255, 128, 32});
  Buffer different_buffer_1({1, 2, 3});
  Buffer different_buffer_2({1, 2, 3, 5, 77, 255});
  Buffer different_buffer_3({255, 128, 32, 5, 77, 255});
  ASSERT_EQ(tested_buffer, tested_buffer);
  ASSERT_EQ(tested_buffer, identical_buffer);
  ASSERT_NE(tested_buffer, different_buffer_1);
  ASSERT_NE(tested_buffer, different_buffer_2);
  ASSERT_NE(tested_buffer, different_buffer_3);
}

TEST(Buffer, copy) {
  Buffer tested_buffer({255, 128, 32});
  Buffer copied_buffer({1, 2, 3, 5, 77, 255});
  ASSERT_NE(tested_buffer, copied_buffer);
  tested_buffer.copy(copied_buffer);
  ASSERT_EQ(tested_buffer, copied_buffer);
  Buffer copied_buffer_2({33, 66, 99, 88, 33, 22, 123});
  tested_buffer.copy(copied_buffer_2);
  ASSERT_EQ(tested_buffer, copied_buffer_2);
  ASSERT_NE(copied_buffer, copied_buffer_2);
  const std::size_t raw_data_size(3);
  uint8_t raw_data[3] = {1, 22, 233};
  tested_buffer.copy(raw_data, raw_data_size);
  ASSERT_EQ(tested_buffer, Buffer(raw_data, raw_data_size));
  std::string data_str("F}A2<");
  tested_buffer.copy(data_str);
  Buffer reference_buffer_str({70, 125, 65, 50, 60});
  ASSERT_EQ(tested_buffer, reference_buffer_str);
}

TEST(Buffer, hash) {
  Buffer buff("qui veut du jus de fruits ?");
  std::hash<Buffer> hasher;
  const std::size_t expected_hash(16352209900986736691U);
  ASSERT_EQ(hasher(buff), expected_hash);
}

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
