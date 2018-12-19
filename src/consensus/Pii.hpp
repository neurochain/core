#ifndef NEURO_SRC_CONSENSUS_PII_HPP
#define NEURO_SRC_CONSENSUS_PII_HPP

#include "common/types.hpp"
#include "common.pb.h"
#include "config.pb.h"
#include "messages.pb.h"
#include "messages/Address.hpp"
#include "ledger/Ledger.hpp"
#include "networking/Networking.hpp"

namespace neuro {
namespace consensus {
using Score = Double;

  
class Addresses {
 public:
  using TransactionID = messages::Transaction *;

  struct Counters {

  };

  class Transactions {
   private:
    const messages::Address _address;
    Score _previous;
    Score _current;
    std::unordered_multimap<messages::Address, Counters> _in;
    std::unordered_multimap<messages::Address, Counters> _out;

   public:
    Transactions(const messages::Address &address);
    bool add(const messages::Output &output);
    bool add(const messages::Input &input);
  };

 private:
  std::unordered_map<messages::Address, std::unique_ptr<AddressTransactions>> _addresses;

 public:
  bool add_block(const messages::Block &block);
};

class Pii {
 public:

 private:
  Transactions _transactions;
  std::shared_ptr<ledger::Ledger> _ledger;
  
 public:
  Pii(const messages::config::Pii &config, std::shared_ptr<ledger::Ledger> ledger,
      std::shared_ptr<networking::Networking> networking):
      _ledger(ledger) {}

  bool add_block(const messages::Block &block);
  // change bool with state [refused, forked, detached, main] when it's merged

  bool add_transaction(const messages::Transaction &transaction);
};

}  // namespace consensus
}  // namespace neuro

#endif /* NEURO_SRC_CONSENSUS_PII_HPP */
