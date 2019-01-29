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
  auto transaction_raw = transaction;
  transaction_raw.clear_signatures();

  // Fill the id which is a required field. This makes the transaction
  // serializable.
  transaction_raw.mutable_id()->set_type(messages::Hash::SHA256);
  transaction_raw.mutable_id()->set_data("");

  Buffer buffer;
  messages::to_buffer(transaction_raw, &buffer);

  for (const auto &input : transaction.inputs()) {
    const auto signature = transaction.signatures(input.signature_id());

    crypto::EccPub ecc_pub;
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

void set_required_fields(messages::Block *block) {
  auto header = block->mutable_header();
  header->mutable_id()->set_type(messages::Hash::SHA256);
  header->mutable_id()->set_data("");
  auto author = header->mutable_author();
  author->mutable_key_pub()->set_raw_data("");
  author->mutable_signature()->set_type(messages::Hash::SHA256);
  author->mutable_signature()->set_data("");
}

bool sign(const crypto::Ecc &keys, messages::Block *block) {
  // Fill the id and the author which are required fields. This makes the block
  // serializable.
  // The block should always be hashed after setting the signature so this
  // should not be overwriting anything
  set_required_fields(block);
  Buffer serialized_block;
  messages::to_buffer(*block, &serialized_block);

  const auto signature = keys.private_key().sign(serialized_block);

  auto header = block->mutable_header();
  auto author = header->mutable_author();
  author->mutable_signature()->set_data(signature.data(), signature.size());
  author->mutable_signature()->set_type(messages::Hash::SHA256);

  keys.public_key().save(author->mutable_key_pub());

  return true;
}

bool verify(const messages::Block &block) {
  auto block_copy = block;
  auto mutable_block = &block_copy;
  mutable_block->mutable_header()->clear_author();

  // Fill the id which is a required field. This makes the transaction
  // serializable.
  set_required_fields(mutable_block);

  Buffer buffer;
  messages::to_buffer(block_copy, &buffer);

  const auto author = block.header().author();
  const auto hash = author.signature().data();
  const Buffer sig(hash.data(), hash.size());

  crypto::EccPub ecc_pub;
  ecc_pub.load(author.key_pub());

  if (!ecc_pub.verify(buffer, sig)) {
    LOG_WARNING << "Wrong signature in block " << block.header().id();
    return false;
  }

  return true;
}

}  // namespace crypto
}  // namespace neuro
