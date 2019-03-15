#include "messages/Address.hpp"
#include <cryptopp/hex.h>
#include <gtest/gtest.h>
#include "crypto/EccPub.hpp"

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

TEST(Address, address) {
  crypto::Ecc ecc;
  crypto::EccPub ecc_pub = ecc.public_key();
  messages::KeyPub key_pub;
  ecc_pub.save(&key_pub);
  Address address0(ecc_pub);
  Address address1(key_pub);
  auto address2 = Address::random();
  for (const auto &address : {address0, address1, address2}) {
    ASSERT_EQ(address.data().size(), 34);
    ASSERT_EQ(address.data()[0], 'N');
  }
  ASSERT_EQ(address0, address1);
  ASSERT_NE(address0, address2);
  ASSERT_NE(address1, address2);
}

}  // namespace test
}  // namespace messages
}  // namespace neuro
