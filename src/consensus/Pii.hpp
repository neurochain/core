#ifndef NEURO_SRC_CONSENSUS_PII_HPP
#define NEURO_SRC_CONSENSUS_PII_HPP

#include "consensus/Config.hpp"
#include "ledger/Ledger.hpp"
#include "messages.pb.h"
#include "messages/Message.hpp"

namespace neuro {
namespace consensus {
namespace tests {
class Pii;
}

using TotalSpent = std::unordered_map<messages::_KeyPub, messages::NCCValue>;
using Balances = std::unordered_map<messages::_KeyPub, messages::Balance>;

class KeyPubs {
 public:
  struct Counters {
    size_t nb_transactions;
    Double enthalpy;
    Counters() : nb_transactions(0), enthalpy(0) {}
  };

  struct Transactions {
    std::unordered_map<messages::_KeyPub, Counters> _in;
    std::unordered_map<messages::_KeyPub, Counters> _out;
  };

 private:
  std::unordered_map<messages::_KeyPub, Transactions> _key_pubs;

 public:
  void add_enthalpy(const messages::_KeyPub &sender,
                    const messages::_KeyPub &recipient, const Double &enthalpy);

  Double get_entropy(const messages::_KeyPub &key_pub) const;

  std::vector<messages::_KeyPub> key_pubs() const;

  friend class tests::Pii;
};

class Pii {
 private:
  KeyPubs _key_pubs;
  std::shared_ptr<ledger::Ledger> _ledger;
  Config _config;

  TotalSpent get_total_spent(const messages::Block &block) const;

  Balances get_balances(const messages::TaggedBlock &tagged_block) const;

  bool add_transaction(const messages::Transaction &transaction,
                       const messages::TaggedBlock &tagged_block,
                       const TotalSpent &total_spent, const Balances &balances);

  Double enthalpy_n() const;
  Double enthalpy_c() const;
  Double enthalpy_lambda() const;

 public:
  Pii(std::shared_ptr<ledger::Ledger> ledger, const consensus::Config &config)
      : _ledger(ledger), _config(config) {}

  ~Pii();

  Config config() const;

  bool add_block(const messages::TaggedBlock &tagged_block);

  std::vector<messages::Pii> get_key_pubs_pii(
      const messages::AssemblyHeight &assembly_height,
      const messages::BranchPath &branch_path);

  friend class tests::Pii;
};

}  // namespace consensus
}  // namespace neuro

#endif /* NEURO_SRC_CONSENSUS_PII_HPP */
