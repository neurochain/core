#ifndef NEURO_SRC_MESSAGES_HASHER_HPP
#define NEURO_SRC_MESSAGES_HASHER_HPP

#include "common/Buffer.hpp"
#include "crypto/EccPub.hpp"
#include "crypto/Hash.hpp"
#include "messages/Message.hpp"

namespace neuro {
namespace messages {

class Hasher : public messages::Hash {
 private:
  std::shared_ptr<Buffer> _data{std::make_shared<Buffer>()};

 public:
  Hasher() {}

  Hasher(const Buffer &data) {
    crypto::hash_sha3_256(data, _data.get());
    this->set_type(Hash::SHA256);
    this->set_data(_data->data(), _data->size());
  }

  Hasher(const crypto::EccPub &ecc_pub) {
    Buffer data;
    ecc_pub.save(&data);
    const auto tmp = crypto::hash_sha3_256(data);
    this->set_type(Hash::SHA256);
    this->set_data(tmp.data(), tmp.size());
  }

  Hasher(const Packet &packet) { to_buffer(packet, _data.get()); }

  std::shared_ptr<Buffer> raw() const { return _data; }
};

}  // namespace messages
}  // namespace neuro

#endif /* NEURO_SRC_MESSAGES_HASHER_HPP */
