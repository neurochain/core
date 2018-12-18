#include "crypto/Sign.hpp"
#include "common/logger.hpp"

namespace neuro {
namespace crypto {

bool sign(const std::vector<const crypto::Ecc *> keys,
          messages::Transaction *transaction) {
  // Fill the id which is a required field. This makes the transaction
  // serializable.
  // The transaction should always be hashed after setting the signature so this
  // should not be overwriting anything
  transaction->mutable_id()->set_type(messages::Hash::SHA256);
  transaction->mutable_id()->set_data("");

  Buffer transaction_serialized;
  messages::to_buffer(*transaction, &transaction_serialized);

  for (const auto &input : *transaction->mutable_inputs()) {
    const auto key = keys[input.key_id()];
    const auto sig = key->private_key().sign(transaction_serialized);
    auto signature = transaction->add_signatures();

    auto hash = signature->mutable_signature();
    hash->set_data(sig.data(), sig.size());
    hash->set_type(messages::Hash::SHA256);

    auto key_pub = signature->mutable_key_pub();
    Buffer tmp;
    key->public_key().save(&tmp);
    key_pub->set_type(messages::KeyType::ECP256K1);

    key_pub->set_raw_data(tmp.data(), tmp.size());
  }

  return true;
}

bool verify(const messages::Transaction &transaction) {
  Buffer bin;
  auto transaction_raw = transaction;
  transaction_raw.clear_signatures();
  transaction_raw.clear_id();
  messages::to_buffer(transaction_raw, &bin);

  for (const auto &input : transaction.inputs()) {
    const auto signature = transaction.signatures(input.signature_id());

    const auto key_pub_raw = signature.key_pub().raw_data();
    Buffer tmp(key_pub_raw.data(), key_pub_raw.size());
    crypto::EccPub ecc_pub(tmp);

    const auto hash = signature.signature().data();
    const Buffer sig(hash.data(), hash.size());

    if (!ecc_pub.verify(bin, sig)) {
      LOG_WARNING << "Wrong signature in transaction " << transaction;
      return false;
    }
  }
  return true;
}

}  // namespace crypto
}  // namespace neuro
