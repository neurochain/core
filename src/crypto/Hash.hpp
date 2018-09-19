#ifndef NEURO_SRC_CRYPTO_HASH_HPP
#define NEURO_SRC_CRYPTO_HASH_HPP

#include "common/Buffer.hpp"

namespace neuro {
namespace crypto {
using Hash = Buffer;

Buffer hash_sha3_256(const Buffer &data);
}  // namespace crypto
}  // namespace neuro

#endif /* NEURO_SRC_CRYPTO_HASH_HPP */
