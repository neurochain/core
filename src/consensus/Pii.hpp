#ifndef NEURO_SRC_CONSENSUS_PII_HPP
#define NEURO_SRC_CONSENSUS_PII_HPP

#include "common.pb.h"
#include "common/types.hpp"
#include "config.pb.h"
#include "ledger/Ledger.hpp"
#include "messages.pb.h"
#include "messages/Address.hpp"
#include "networking/Networking.hpp"

namespace neuro {
namespace consensus {

class Addresses {
 public:
  struct Counters {
    size_t nb_transactions;
    Double enthalpy;
    Counters() : nb_transactions(0), enthalpy(0) {}
  };

  struct Transactions {
    std::unordered_map<messages::Address, Counters> _in;
    std::unordered_map<messages::Address, Counters> _out;
  };

 private:
  std::unordered_map<messages::Address, Transactions> _addresses;

 public:
  void add_enthalpy(const messages::Address &sender,
                    const messages::Address &recipient, Double enthalpy);

  Double get_entropy(const messages::Address &address) const;
};

class Pii {
 public:
 private:
  Addresses _addresses;
  std::shared_ptr<ledger::Ledger> _ledger;

  bool get_enthalpy(const messages::Transaction &transaction,
                    const messages::TaggedBlock &tagged_block,
                    const messages::Hash &previous_assembly_id,
                    const messages::Address &sender,
                    const messages::Address &recipient, Double *enthalpy) const;

  bool get_recipients(const messages::Transaction &transaction,
                      std::vector<messages::Address> *recipients) const;

  bool get_senders(const messages::Transaction &transaction,
                   const messages::TaggedBlock &tagged_block,
                   std::vector<messages::Address> *senders) const;

  bool add_transaction(const messages::Transaction &transaction,
                       const messages::TaggedBlock &tagged_block,
                       const messages::Hash &previous_assembly_id);

  bool get_score(const messages::Address &address,
                 const messages::Hash &assembly_id, Double *score) const;

  Double enthalpy_n() const;
  Double enthalpy_c() const;
  Double enthalpy_lambda() const;

 public:
  Pii(const messages::config::Pii &config,
      std::shared_ptr<ledger::Ledger> ledger,
      std::shared_ptr<networking::Networking> networking)
      : _ledger(ledger) {}

  bool add_block(const messages::TaggedBlock &tagged_block,
                 const messages::Hash &previous_assembly_id);
};

}  // namespace consensus
}  // namespace neuro

#endif /* NEURO_SRC_CONSENSUS_PII_HPP */
