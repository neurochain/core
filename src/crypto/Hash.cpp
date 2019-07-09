
#include <cryptopp/sha3.h>

#include "crypto/Hash.hpp"

namespace neuro {
namespace crypto {

void hash_sha3_256(const Buffer &data, Buffer *out) {
  out->resize(CryptoPP::SHA3_256::DIGESTSIZE);

  CryptoPP::SHA3_256().CalculateDigest(out->data(), data.data(), data.size());
}

Buffer hash_sha3_256(const Buffer &data) {
  Buffer digest3(CryptoPP::SHA3_256::DIGESTSIZE, 0);
  hash_sha3_256(data, &digest3);
  return digest3;
}

Buffer hash_sha3_256(const messages::Packet &packet) {
  Buffer buffer;
  messages::to_buffer(packet, &buffer);
  return hash_sha3_256(buffer);
}

}  // namespace crypto
}  // namespace neuro
