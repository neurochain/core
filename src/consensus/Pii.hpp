#ifndef NEURO_SRC_CONSENSUS_PII_HPP
#define NEURO_SRC_CONSENSUS_PII_HPP

#include "common.pb.h"
#include "messages.pb.h"
#include "messages/Address.hpp"

namespace neuro {
namespace consensus {

class Transactions {
 public:
  using TransactionID = messages::Transaction *;
  using AddressID = messages::Address *;

  class Address {
   private:
    const crypto::Address _address;
    Score _previous;
    Score _current;
    std::unordered_multimap<const AddressID, const TransactionID> _in;
    std::unordered_multimap<const AddressID, const TransactionID> _out;

   public:
    Address(const crypto::Address &address);
    bool add_incoming(const AddressID address_id,
                      const TransactionID transaction_id);
    bool add_outgoing(const AddressID address_id,
                      const TransactionID transaction_id);
  };

 private:
  std::vector<std::unique_ptr<messages::Transaction>> _transactions;
  std::unordered_set<std::unique_ptr<messages::Address>> _addresss;

 public:
  bool add_block(const messages::Block &block);
};

class Pii {
 public:
  using Score = Double;

 private:
  Transactions _transactions;

 public:
  Pii(const config::Pii &config, std::shared_ptr<Ledger> ledger,
      std::shared_ptr<Networking> networking) {}

  bool add_block(const messages::Block &block);
  // change bool with state [refused, forked, detached, main] when it's merged

  bool add_transaction(const messages::Transaction &transaction);
};

}  // namespace consensus
}  // namespace neuro
}  // neuro

#endif /* NEURO_SRC_CONSENSUS_PII_HPP */
