#include "crypto/Sign.hpp"
#include "common/logger.hpp"
#include "messages/Address.hpp"

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

  Buffer serialized_transaction;
  messages::to_buffer(*transaction, &serialized_transaction);

  for (const auto &input : *transaction->mutable_inputs()) {
    const auto key = keys[input.signature_id()];
    const auto signature = key->private_key().sign(serialized_transaction);
    auto input_signature = transaction->add_signatures();

    input_signature->mutable_signature()->set_data(signature.data(),
                                                   signature.size());
    input_signature->mutable_signature()->set_type(messages::Hash::SHA256);

    key->public_key().save(input_signature->mutable_key_pub());
  }

  return true;
}

bool verify(const messages::Transaction &transaction) {
  auto transaction_copy = transaction;
  transaction_copy.clear_signatures();

  // Fill the id which is a required field. This makes the transaction
  // serializable.
  transaction_copy.mutable_id()->set_type(messages::Hash::SHA256);
  transaction_copy.mutable_id()->set_data("");

  Buffer buffer;
  messages::to_buffer(transaction_copy, &buffer);

  for (const auto &input : transaction.inputs()) {
    const auto signature = transaction.signatures(input.signature_id());

    crypto::EccPub ecc_pub;
    LOG_DEBUG << "signature key pub " << signature.key_pub();
    ecc_pub.load(signature.key_pub());

    const auto hash = signature.signature().data();
    const Buffer sig(hash.data(), hash.size());

    if (!ecc_pub.verify(buffer, sig)) {
      LOG_WARNING << "Wrong signature in transaction " << transaction;
      return false;
    }
  }
  return true;
}

bool sign(const crypto::Ecc &keys, messages::Block *block) {
  // Actually we never sign a block directly but the denunciation of a block.
  // It makes it easier for others to denunce us if we mess up.
  // And the denunciation contains the block id so it is pretty much the same.
  auto denunciation = messages::Denunciation(*block);
  sign(keys, &denunciation);
  block->mutable_header()->mutable_author()->CopyFrom(
      denunciation.block_author());
  return true;
}

bool verify(const messages::Block &block) {
  return verify(messages::Denunciation(block));
}

bool sign(const crypto::Ecc &keys, messages::Denunciation *denunciation) {
  auto author = denunciation->mutable_block_author();
  messages::set_default(author);
  Buffer serialized;
  messages::to_buffer(*denunciation, &serialized);

  const auto signature = keys.private_key().sign(serialized);
  author->mutable_signature()->set_data(signature.data(), signature.size());
  author->mutable_signature()->set_type(messages::Hash::SHA256);
  keys.public_key().save(author->mutable_key_pub());

  return true;
}

bool verify(const messages::Denunciation &denunciation) {
  auto denunciation_copy = denunciation;
  messages::set_default(denunciation_copy.mutable_block_author());

  Buffer buffer;
  messages::to_buffer(denunciation_copy, &buffer);

  const auto author = denunciation.block_author();
  const auto hash = author.signature().data();
  const Buffer sig(hash.data(), hash.size());

  crypto::EccPub ecc_pub;
  ecc_pub.load(author.key_pub());

  if (!ecc_pub.verify(buffer, sig)) {
    LOG_WARNING << "Wrong signature in denunciation with block id "
                << denunciation.block_id();
    return false;
  }

  return true;
}

}  // namespace crypto
}  // namespace neuro
