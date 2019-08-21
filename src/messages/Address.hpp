#ifndef NEURO_SRC_MESSAGES_ADDRESS_HPP
#define NEURO_SRC_MESSAGES_ADDRESS_HPP

#include <cryptopp/algebra.h>
#include <cryptopp/integer.h>
#include <vector>

#include "common.pb.h"
#include "common/Buffer.hpp"
#include "common/logger.hpp"
#include "crypto/Ecc.hpp"
#include "crypto/KeyPub.hpp"

namespace neuro {
namespace messages {

std::string encode_base58(const CryptoPP::Integer &num,
                          const std::string &version = "");

std::string encode_base58(const Buffer &buffer,
                          const std::string &version = "");

std::string encode_base58(const std::string &data,
                          const std::string &version = "");

CryptoPP::Integer decode_base58(const std::string &encoded_number);

class Address : public _Address {
 private:
  std::size_t _hash_size = 30;
  std::size_t _checksum_size = 4;

  void init(const messages::_KeyPub &key_pub);

 public:
  Address();
  Address(const messages::_KeyPub &key_pub);
  Address(const crypto::KeyPub &ecc_pub);
  Address(const messages::_Address &address);
  Address(const std::string &address);

  static Address random();

  bool verify() const;

  operator bool() const;
};

using Addresses = std::vector<Address>;

}  // namespace messages
}  // namespace neuro

#endif /* NEURO_SRC_MESSAGES_ADDRESS_HPP */
