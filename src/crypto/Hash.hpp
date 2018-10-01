#ifndef NEURO_SRC_CRYPTO_HASH_HPP
#define NEURO_SRC_CRYPTO_HASH_HPP

#include "common/Buffer.hpp"
#include "common/types.hpp"

namespace neuro {
namespace crypto {

  class Hash : public Buffer {
  public:
    Hash(const Buffer &data): Buffer(CryptoPP::SHA3_256::DIGESTSIZE, uint8_t{0}) {
      CryptoPP::SHA3_256().CalculateDigest(this->data(), data.data(),
					   data.size());
    }
  };



}  // namespace crypto
}  // namespace neuro

#endif /* NEURO_SRC_CRYPTO_HASH_HPP */
