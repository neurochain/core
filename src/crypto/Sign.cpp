#include "crypto/Sign.hpp"

namespace neuro {
namespace crypto {

bool sign(const std::vector<const EccPriv *> key_privs,
          messages::Transaction *transaction) {
  Buffer transaction_serialized;
  messages::to_buffer(*transaction, &transaction_serialized);

  for (const auto key_priv : key_privs) {
    const auto sig = key_priv->sign(transaction_serialized);
    auto signature = transaction->add_signatures();
    signature->set_data(sig.data(), sig.size());
  }
  return true;
}

}  // namespace crypto
}  // namespace neuro
