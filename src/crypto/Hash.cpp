
#include <sha3.h>

#include "crypto/Hash.hpp"

namespace neuro {
namespace crypto {

Buffer hash_sha3_256(const Buffer &data) {
  Buffer digest3(CryptoPP::SHA3_256::DIGESTSIZE, 0);
  CryptoPP::SHA3_256().CalculateDigest(digest3.data(), data.data(),
                                       data.size());

  return digest3;
}

}  // namespace crypto
}  // namespace neuro
