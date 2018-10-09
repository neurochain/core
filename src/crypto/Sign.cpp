#include "crypto/Sign.hpp"

namespace neuro {
namespace crypto {

bool sign (const std::vector<const EccPriv *>key_privs, messages::Transaction *transaction) {
  Buffer transaction_serialized;
  messages::to_buffer(*transaction, &transaction_serialized);

  for (const auto key_priv : key_privs) {
    const auto sig = key_priv->sign(transaction_serialized);
    auto signature = transaction->add_signatures();
    //signature->set_data(sig.data(), sig.size());
  }
  return true;
}

bool verify (const messages::Transaction &transaction) {

  Buffer bin;
  auto transaction_raw = transaction;
  transaction_raw.clear_signatures();
  messages::to_buffer(transaction_raw, &bin);

  for (const auto& input : transaction.inputs()) {
    //const auto signature = transaction.signatures(intput.signature_id());
    
  }
}

  
}  // crypto
}  // neuro
