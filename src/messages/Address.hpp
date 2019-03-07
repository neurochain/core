#ifndef NEURO_SRC_MESSAGES_ADDRESS_HPP
#define NEURO_SRC_MESSAGES_ADDRESS_HPP

#include "common.pb.h"
#include "common/logger.hpp"
#include "crypto/Ecc.hpp"
#include "messages/Hasher.hpp"

namespace neuro {
namespace messages {

class Address : public _Address {
 private:
  std::size_t hash_size = 10;
  std::size_t checksum_size = 2;

  void init(const messages::KeyPub &key_pub) {
    auto key = crypto::hash_sha3_256(key_pub);
    auto checksum = crypto::hash_sha3_256(key);
    checksum.resize(checksum_size);
    key.append(checksum);
    set_data(key.data(), key.size());
  }

 public:
  Address(const messages::KeyPub &key_pub) { init(key_pub); }

  Address(const crypto::EccPub &ecc_pub) {
    messages::KeyPub key_pub;
    ecc_pub.save(&key_pub);
    init(key_pub);
  }

  Address(const messages::_Address &address) : _Address(address) {}

  Address() : _Address() {}

  static Address random() {
    crypto::Ecc ecc;
    return Address(ecc.public_key());
  }

  bool verify() const {
    if (!has_data()) {
      return false;
    }

    if (data().size() != (hash_size + checksum_size)) {
      return false;
    }

    const Buffer key(data().substr(0, hash_size));
    const Buffer checksum_from_key(data().substr(hash_size, checksum_size));
    auto checksum_computed = crypto::hash_sha3_256(key);

    if (checksum_computed != checksum_from_key) {
      LOG_INFO << "Key checksum error";
      return false;
    }

    return true;
  }

  operator bool() const { return verify(); }
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
