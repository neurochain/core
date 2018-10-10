#ifndef NEURO_SRC_CRYPTO_SIGN_HPP
#define NEURO_SRC_CRYPTO_SIGN_HPP

#include "crypto/Ecc.hpp"
#include "messages.pb.h"
#include "messages/Message.hpp"

namespace neuro {
namespace crypto {

bool sign(const std::vector<const crypto::Ecc *> keys,
          messages::Transaction *transaction);

bool verify(const messages::Transaction &transaction);

}  // namespace crypto
}  // namespace neuro

#endif /* NEURO_SRC_CRYPTO_SIGN_HPP */
