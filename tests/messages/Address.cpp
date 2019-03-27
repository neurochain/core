#include "messages/Address.hpp"
#include <cryptopp/hex.h>
#include <gtest/gtest.h>
#include <random>
#include "crypto/KeyPub.hpp"

namespace neuro {
namespace messages {
namespace test {

TEST(Address, encode_base58) {
  // There is no 0 in base58
  std::map<int, std::string> test_cases = {
      {0, "1"},        {1, "2"},       {57, "z"},
      {58, "21"},      {2 * 58, "31"}, {57 * 58 + 57, "zz"},
      {58 * 58, "211"}};
  for (const auto &[i, result] : test_cases) {
    ASSERT_EQ(encode_base58(static_cast<CryptoPP::Integer>(i), ""), result);
  }

  ASSERT_EQ(encode_base58(static_cast<CryptoPP::Integer>(0l), "TITI"), "TITI1");
}

TEST(Address, address) {
  crypto::Ecc ecc;
  crypto::KeyPub ecc_pub = ecc.key_pub();
  messages::_KeyPub key_pub;
  ecc_pub.save(&key_pub);
  Address address0(ecc_pub);
  Address address1(key_pub);
  auto address2 = Address::random();
  for (const auto &address : {address0, address1, address2}) {
    ASSERT_EQ(address.data().size(), 34);
    ASSERT_EQ(address.data()[0], 'N');
    ASSERT_TRUE(address.verify());
  }
  ASSERT_EQ(address0, address1);
  ASSERT_NE(address0, address2);
  ASSERT_NE(address1, address2);
}

TEST(Address, verify) {
  auto address = Address::random();
  ASSERT_TRUE(address.verify());
  for (int i = 0; i < 100; i++) {
    address = Address::random();
    ASSERT_TRUE(address.verify());
    int j = rand() % address.data().size();
    (*address.mutable_data())[j]++;
    ASSERT_FALSE(address.verify());
  }
}

}  // namespace test
}  // namespace messages
}  // namespace neuro
