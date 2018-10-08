#include "crypto/Sign.hpp"

namespace neuro {
namespace crypto {

bool sign (const EccPriv &key_priv, messages::Transaction *transaction) {
  Buffer tmp;
  messages::to_buffer(*transaction, &tmp);
  const auto sig = key_priv.sign(tmp);
  auto signature = transaction->add_signatures();
  signature->set_data(sig.data(), sig.size());

  return true;
}

}  // crypto
}  // neuro
