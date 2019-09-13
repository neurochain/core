#include "ledger/Transaction.hpp"

namespace neuro::ledger {

messages::TaggedTransaction Transaction::make_tagged_detached(
    const neuro::messages::Transaction &transaction) {
  messages::TaggedTransaction tagged_transaction;
  tagged_transaction.set_is_coinbase(false);
  tagged_transaction.mutable_transaction()->CopyFrom(transaction);
  tagged_transaction.set_fee_per_bytes(
      double(transaction.fees().value()) / transaction.ByteSizeLong());
  return tagged_transaction;
}

messages::TaggedTransaction
Transaction::make_tagged(const messages::Hash &id,
                         const messages::Transaction &transaction) {
  auto tagged_transaction = make_tagged_detached(transaction);
  tagged_transaction.mutable_block_id()->CopyFrom(id);
  return tagged_transaction;
}

messages::TaggedTransaction Transaction::make_tagged_coinbase(
    const messages::Hash &id,
    const messages::Transaction &coinbase_transaction) {
  messages::TaggedTransaction tagged_transaction =
      make_tagged(id, coinbase_transaction);
  tagged_transaction.set_is_coinbase(true);
  return tagged_transaction;
}

} // namespace neuro::ledger
