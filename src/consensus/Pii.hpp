#ifndef NEURO_SRC_CONSENSUS_PII_HPP
#define NEURO_SRC_CONSENSUS_PII_HPP

#include "consensus/Config.hpp"
#include "ledger/Ledger.hpp"
#include "messages.pb.h"
#include "messages/Address.hpp"

namespace neuro {
namespace consensus {
namespace tests {
class Pii;
}

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

  std::vector<messages::Address> addresses() const;

  friend class tests::Pii;
};

class Pii {
 private:
  Addresses _addresses;
  std::shared_ptr<ledger::Ledger> _ledger;
  Config _config;

  bool get_enthalpy(const messages::Transaction &transaction,
                    const messages::TaggedBlock &tagged_block,
                    const messages::Address &sender,
                    const messages::Address &recipient, Double *enthalpy) const;

  bool get_recipients(const messages::Transaction &transaction,
                      std::vector<messages::Address> *recipients) const;

  bool get_senders(const messages::Transaction &transaction,
                   const messages::TaggedBlock &tagged_block,
                   std::vector<messages::Address> *senders) const;

  bool add_transaction(const messages::Transaction &transaction,
                       const messages::TaggedBlock &tagged_block);

  Double enthalpy_n() const;
  Double enthalpy_c() const;
  Double enthalpy_lambda() const;

 public:
  Pii(std::shared_ptr<ledger::Ledger> ledger, const consensus::Config &config)
      : _ledger(ledger), _config(config) {}

  Config config() const;

  bool add_block(const messages::TaggedBlock &tagged_block);

  std::vector<messages::Pii> get_addresses_pii(
      const messages::AssemblyHeight &assembly_height,
      const messages::BranchPath &branch_path);

  friend class tests::Pii;
};

}  // namespace consensus
}  // namespace neuro

#endif /* NEURO_SRC_CONSENSUS_PII_HPP */
