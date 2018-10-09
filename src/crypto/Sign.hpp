#ifndef NEURO_SRC_CRYPTO_SIGN_HPP
#define NEURO_SRC_CRYPTO_SIGN_HPP

#include "messages.pb.h"
#include "messages/Message.hpp"
#include "crypto/Ecc.hpp"

namespace neuro {
namespace crypto {

bool sign (const std::vector<const crypto::Ecc *>keys, messages::Transaction *transaction);


bool verify (const messages::Transaction &transaction);
  
}  // crypto
}  // neuro

#endif /* NEURO_SRC_CRYPTO_SIGN_HPP */
