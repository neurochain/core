#ifndef NEURO_SRC_CRYPTO_SIGN_HPP
#define NEURO_SRC_CRYPTO_SIGN_HPP

#include "crypto/EccPriv.hpp"
#include "messages.pb.h"
#include "messages/Message.hpp"

namespace neuro {
namespace crypto {

bool sign(const std::vector<const EccPriv *> key_privs,
          messages::Transaction *transaction);

}  // namespace crypto
}  // namespace neuro

#endif /* NEURO_SRC_CRYPTO_SIGN_HPP */
