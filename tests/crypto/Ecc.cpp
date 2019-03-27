#include "crypto/Ecc.hpp"
#include <dsa.h>
#include <gtest/gtest.h>
#include "common/logger.hpp"

namespace neuro {
namespace crypto {
namespace test {

TEST(Ecc, save_load_file) {
  crypto::Ecc keys0;
  ASSERT_TRUE(keys0.save("test_keys.priv", "test_keys.pub"));

  crypto::Ecc keys1("test_keys.priv", "test_keys.pub");

  ASSERT_EQ(keys0, keys1);
}

TEST(Ecc, check_key_value_expectations) {
  crypto::Ecc keys0;
  keys0.save("test_keys.priv", "test_keys.pub");
  crypto::Ecc keys1("test_keys.priv", "test_keys.pub");
  std::ifstream pub_key_file("test_keys.pub", std::ifstream::binary);
  ASSERT_TRUE(pub_key_file.is_open());
  std::vector<uint8_t> buffer1(std::istreambuf_iterator<char>(pub_key_file),
                               {});
  ASSERT_FALSE(buffer1.empty());
  const auto raw_data = keys1.public_key().save();
  ASSERT_EQ(buffer1, raw_data);
  Buffer buffer2;
  keys1.public_key().save(&buffer2);
  ASSERT_EQ(buffer1, buffer2);
}

TEST(Ecc, save_load_buffer) {
  const crypto::Ecc keys0;
  crypto::Ecc keys1;
  Buffer buff;

  keys0.private_key().save(&buff);
  keys1.mutable_private_key()->load(buff);

  keys0.public_key().save(&buff);
  *keys1.mutable_public_key() = buff;

  ASSERT_EQ(keys0, keys1);
}

TEST(Ecc, save_load_protobuf) {
  crypto::Ecc ecc;
  messages::KeyPub key_pub;

  ASSERT_TRUE(ecc.public_key().save(&key_pub));
  messages::KeyPub ecc_pub(key_pub);
  ASSERT_EQ(ecc.public_key(), ecc_pub);
}

TEST(Ecc, save_load_protobuf_as_hex) {
  const crypto::Ecc keys0;
  crypto::Ecc ecc;
  messages::KeyPub key_pub;

  ASSERT_TRUE(ecc.public_key().save_as_hex(&key_pub));
  crypto::EccPub ecc_pub(key_pub);
  ASSERT_EQ(ecc.public_key(), ecc_pub);
}

TEST(Ecc, sign_verify) {
  // echo -n "Hola mundo desde NeuroChainTech" |openssl dgst -sha256 -sign
  // test_sign.priv -keyform DER |hexdump
  const uint8_t derSignature[] = {
      0x30, 0x46, 0x02, 0x21, 0x00, 0xb2, 0xb2, 0x95, 0xc4, 0x50, 0xbb, 0x3f,
      0x68, 0xab, 0x48, 0xf9, 0xb3, 0xfd, 0x8e, 0x6a, 0x38, 0x16, 0x87, 0x4e,
      0x45, 0xd3, 0xe5, 0x1d, 0x2c, 0xb4, 0x41, 0x35, 0x73, 0x46, 0xd4, 0x6c,
      0xc8, 0x02, 0x21, 0x00, 0xd8, 0xe5, 0xf7, 0xe8, 0xf6, 0x6e, 0xbd, 0x97,
      0x57, 0x4d, 0x84, 0x99, 0x7e, 0x41, 0x8e, 0x43, 0x64, 0x17, 0x9e, 0x2c,
      0x86, 0xf3, 0x6d, 0xdd, 0x9c, 0xb7, 0x46, 0x11, 0x58, 0x85, 0xa7, 0x3b};

  Buffer signature(0x40 /*DSA_P1363 signature size*/, 0);
  CryptoPP::DSAConvertSignatureFormat(
      (byte *)signature.data(), signature.size(), CryptoPP::DSA_P1363,
      derSignature, sizeof(derSignature), CryptoPP::DSA_DER);

  // Hola mundo desde NeuroChainTech
  const Buffer buf_msg("Hola mundo desde NeuroChainTech");

  const crypto::Ecc keys("test_sign.priv", "test_sign.pub");
  const EccPub pub_key = keys.public_key();
  ASSERT_TRUE(pub_key.verify(buf_msg, signature));
}

}  // namespace test
}  // namespace crypto
}  // namespace neuro
