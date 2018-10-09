#ifndef NEURO_SRC_MESSAGES_HASHER_HPP
#define NEURO_SRC_MESSAGES_HASHER_HPP

#include "common/Buffer.hpp"
#include "crypto/EccPub.hpp"
#include "crypto/Hash.hpp"

namespace neuro {
namespace messages {

class Hasher : public messages::Hash {
 public:
  Hasher(const Buffer &data) {
    const auto tmp = crypto::hash_sha3_256(data);
    this->set_type(Hash::SHA256);
    this->set_data(tmp.data(), tmp.size());
  }

  Hasher(const crypto::EccPub &ecc_pub) {
    Buffer data;
    ecc_pub.save(&data);
    const auto tmp = crypto::hash_sha3_256(data);
    this->set_type(Hash::SHA256);
    this->set_data(tmp.data(), tmp.size());
  }
};

}  // namespace messages
}  // namespace neuro

#endif /* NEURO_SRC_MESSAGES_HASHER_HPP */
