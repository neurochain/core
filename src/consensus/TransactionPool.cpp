#include "TransactionPool.hpp"
#include "ledger/Ledger.hpp"

namespace neuro {
namespace consensus {

TransactionPool::TransactionPool(std::shared_ptr<ledger::Ledger> ledger)
    : _ledger(ledger) {
  // ctor
}

bool TransactionPool::add_transactions(const messages::Transaction &transaction) {
  if (transaction.has_id()) {
    messages::Transaction oldtrans;
    auto id_transaction = transaction.id();
    if (_ledger->get_transaction(id_transaction, &oldtrans)) {
      return false;
    }
  }

  messages::Transaction trans(transaction);
  trans.clear_block_id();
  trans.clear_id();

  Buffer buf;
  messages::to_buffer(trans, &buf);
  messages::Address id(buf);

  trans.mutable_id()->CopyFrom(id);
  _ledger->add_transaction(trans);
  return true;
}

void TransactionPool::delete_transactions(const messages::Transaction &transaction) {
  _ledger->delete_transaction(transaction.id());
}

bool TransactionPool::build_block(messages::Block &block,
                                  messages::BlockHeight height,
                                  const crypto::Ecc &author, uint64_t rewarde) {
  /// TO DO

  // Get author pubkey and address ---oliv
  Buffer key_pub_data;
  author.public_key().save(&key_pub_data);

  messages::KeyPub author_keypub;
  author_keypub.set_type(messages::KeyType::ECP256K1);
  author_keypub.set_raw_data(key_pub_data.data(), key_pub_data.size());

  messages::Address addr(key_pub_data);

  messages::BlockHeader *blockheader = block.mutable_header();
  blockheader->mutable_author()->CopyFrom(author_keypub);

  messages::Block last_block;
  _ledger->get_block(_ledger->height(), &last_block);

  blockheader->mutable_previous_block_hash()->CopyFrom(
      last_block.header().id());
  blockheader->set_height(height);
  blockheader->mutable_timestamp()->set_data(std::time(nullptr));

  messages::Transaction *coinsbase_transaction = block.add_transactions();

  messages::NCCSDF ncc;
  ncc.set_value(rewarde);

  coinbase(coinsbase_transaction, addr, ncc);

  _ledger->get_transaction_pool(block);

  //! signed block #block1
  return false;
}

void TransactionPool::coinbase(messages::Transaction *transaction, const messages::Hasher &addr,
              const messages::NCCSDF &ncc) {
  auto input = transaction->add_inputs();

  auto input_id = input->mutable_id();
  input_id->set_type(messages::Hash::SHA256);
  input_id->set_data("");
  input->set_output_id(0);

  auto output = transaction->add_outputs();
  output->mutable_address()->CopyFrom(addr);
  output->mutable_value()->CopyFrom(ncc);
  transaction->mutable_fees()->set_value(0);  //"0");
}
}  // namespace consensus
}  // namespace neuro
