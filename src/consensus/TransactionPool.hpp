#ifndef TRANSACTIONPOOL_H
#define TRANSACTIONPOOL_H

#include <memory>
#include "crypto/Ecc.hpp"
#include "crypto/EccPub.hpp"
#include "messages.pb.h"
#include "messages/Address.hpp"
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

  bool add_transactions(const messages::Transaction &transaction);
  void delete_transactions(const messages::Hash &transaction);

  void add_transactions(
      const google::protobuf::RepeatedPtrField<neuro::messages::Transaction>
          &transactions);
  void delete_transactions(
      const google::protobuf::RepeatedPtrField<neuro::messages::Transaction>
          &transactions);

  bool build_block(messages::Block *block, messages::BlockHeight height,
                   const crypto::Ecc *author, uint64_t reward);
  void coinbase(messages::Transaction *transaction,
                const messages::Address &addr, const messages::NCCSDF &ncc);
};

}  // namespace consensus
}  // namespace neuro
#endif  // TRANSACTIONPOOL_H
