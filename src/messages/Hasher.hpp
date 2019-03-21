#ifndef NEURO_SRC_MESSAGES_HASHER_HPP
#define NEURO_SRC_MESSAGES_HASHER_HPP

#include "common/Buffer.hpp"
#include "crypto/Ecc.hpp"
#include "crypto/KeyPub.hpp"
#include "crypto/Hash.hpp"
#include "messages/Message.hpp"

namespace neuro {
namespace messages {

class Hasher : public messages::Hash {
 public:
  Hasher() {}

  void from_buffer(const Buffer &data) {
    Buffer hash;
    crypto::hash_sha3_256(data, &hash);
    this->set_type(Hash::SHA256);
    this->set_data(hash.data(), hash.size());
  }

  explicit Hasher(const Buffer &data) { from_buffer(data); }

  explicit Hasher(const crypto::KeyPub &ecc_pub) {
    Buffer data;
    ecc_pub.save(&data);
    const auto tmp = crypto::hash_sha3_256(data);
    this->set_type(Hash::SHA256);
    this->set_data(tmp.data(), tmp.size());
  }

  explicit Hasher(const Packet &packet) {
    Buffer data;
    to_buffer(packet, &data);
    from_buffer(data);
  }

  static Hasher random() {
    crypto::Ecc ecc;
    return Hasher(ecc.key_pub());
  }

  std::string raw() const { return this->data(); }
};

}  // namespace messages
}  // namespace neuro

#endif /* NEURO_SRC_MESSAGES_HASHER_HPP */
