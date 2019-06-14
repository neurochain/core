#ifndef NEURO_SRC_CRYPTO_SIGN_HPP
#define NEURO_SRC_CRYPTO_SIGN_HPP

#include "crypto/Ecc.hpp"
#include "messages.pb.h"
#include "messages/Message.hpp"

namespace neuro {
namespace crypto {

bool sign(const std::vector<const crypto::Ecc *>& keys,
          messages::Transaction *transaction);

bool verify(const messages::Transaction &transaction);

bool sign(const crypto::Ecc &keys, messages::Block *block);

bool verify(const messages::Block &block);

bool sign(const crypto::Ecc &keys, messages::Denunciation *denunciation);

bool verify(const messages::Denunciation &denunciation);

}  // namespace crypto
}  // namespace neuro

#endif /* NEURO_SRC_CRYPTO_SIGN_HPP */
