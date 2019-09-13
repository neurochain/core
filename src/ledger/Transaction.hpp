#ifndef NEURO_SRC_LEDGER_TRANSACTION_HPP
#define NEURO_SRC_LEDGER_TRANSACTION_HPP

#include "messages.pb.h"

namespace neuro::ledger::Transaction {
messages::TaggedTransaction
make_tagged_detached(const neuro::messages::Transaction &transaction);
messages::TaggedTransaction
make_tagged(const messages::Hash &id, const messages::Transaction &transaction);
messages::TaggedTransaction
make_tagged_coinbase(const messages::Hash &id,
                     const messages::Transaction &transaction);

} // namespace neuro::ledger::Transaction

#endif /* NEURO_SRC_LEDGER_TRANSACTION_HPP */
