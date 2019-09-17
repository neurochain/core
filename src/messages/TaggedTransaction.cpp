#include "messages/TaggedTransaction.hpp"

namespace neuro::messages {

TaggedTransaction::TaggedTransaction(const messages::Transaction& transaction) {
  set_is_coinbase(false);
  mutable_transaction()->CopyFrom(transaction);
  set_fee_per_bytes(double(transaction.fees().value()) / transaction.ByteSizeLong());
}

TaggedTransaction::TaggedTransaction(const messages::Hash &id,
                                     const messages::Transaction &transaction,
                                     bool is_coinbase) : TaggedTransaction(transaction){
  mutable_block_id()->CopyFrom(id);
  set_is_coinbase(is_coinbase);
}

} // namespace neuro::messages
