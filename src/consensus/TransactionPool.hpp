#ifndef TRANSACTIONPOOL_H
#define TRANSACTIONPOOL_H

#include <memory>
#include "crypto/Ecc.hpp"
#include "crypto/EccPub.hpp"
#include "messages.pb.h"
#include "messages/Message.hpp"

namespace neuro {
namespace ledger {
class Ledger;
}  // namespace ledger
namespace consensus {

class TransactionPool {
 private:
  std::shared_ptr<ledger::Ledger> _ledger;

 public:
  TransactionPool(std::shared_ptr<ledger::Ledger> ledger);

  bool add_transactions(messages::Transaction &transaction);
  void delete_transactions(messages::Transaction &transaction);

  bool build_block(messages::Block &block, messages::BlockHeight height,
                   crypto::Ecc &author, uint64_t rewarde = 10);
  void coinbase(messages::Transaction *transaction,
                const messages::Address &addr, const messages::NCCSDF &ncc);
};

}  // namespace consensus
}  // namespace neuro
#endif  // TRANSACTIONPOOL_H
