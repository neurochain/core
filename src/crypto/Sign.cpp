#include "crypto/Sign.hpp"

namespace neuro {
namespace crypto {

bool sign (const EccPriv &key_priv, const int input_index, messages::Transaction *transaction) {
  Buffer tmp;
  messages::to_buffer(*transaction, &tmp);
  const auto sig = key_priv.sign(tmp);
  auto signature = transaction->mutable_inputs(input_index)->mutable_signature();
  signature->set_data(sig.data(), sig.size());

  return true;
}

}  // crypto
}  // neuro
