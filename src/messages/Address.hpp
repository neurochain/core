#ifndef NEURO_SRC_MESSAGES_ADDRESS_HPP
#define NEURO_SRC_MESSAGES_ADDRESS_HPP

#include <cryptopp/integer.h>
#include "common.pb.h"
#include "common/logger.hpp"
#include "crypto/Ecc.hpp"
#include "messages/Hasher.hpp"

namespace neuro {
namespace messages {

std::string encode_base58(const CryptoPP::Integer &num,
                          const std::string &version);

std::string encode_base58(const Buffer &buffer, const std::string &version);

class Address : public _Address {
 private:
  std::size_t _hash_size = 10;
  std::size_t _checksum_size = 2;
  std::size_t _address_size = 34;

  void init(const messages::KeyPub &key_pub);

 public:
  Address(const messages::KeyPub &key_pub);

  Address(const crypto::EccPub &ecc_pub);

  Address(const messages::_Address &address);

  Address();

  static Address random();

  bool verify() const;

  operator bool() const;
};

}  // namespace messages
}  // namespace neuro

namespace std {
template <>
struct hash<neuro::messages::Address> {
  size_t operator()(const neuro::messages::Address &address) const {
    return hash<string>()(to_json(address));
  }
};
}  // namespace std

#endif /* NEURO_SRC_MESSAGES_ADDRESS_HPP */
