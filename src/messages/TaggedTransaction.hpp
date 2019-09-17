#ifndef NEURO_SRC_MESSAGES_TRANSACTION_HPP
#define NEURO_SRC_MESSAGES_TRANSACTION_HPP

#include "messages.pb.h"

namespace neuro::messages {
class TaggedTransaction : public _TaggedTransaction {
public:
  TaggedTransaction() = default;
  explicit TaggedTransaction(const messages::Transaction &transaction);
  explicit TaggedTransaction(const messages::Hash &id,
                             const messages::Transaction &transaction,
                             bool is_coinbase = false);
};

} // namespace neuro::messages

#endif /* NEURO_SRC_LEDGER_TRANSACTION_HPP */
