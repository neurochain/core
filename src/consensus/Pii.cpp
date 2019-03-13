#include "consensus/Pii.hpp"

namespace neuro {
namespace consensus {

Double Pii::enthalpy_n() const { return 1; }

Double Pii::enthalpy_c() const { return 1; }

Double Pii::enthalpy_lambda() const { return 0.5; }

Pii::~Pii() { mpfr_free_cache(); }

bool Pii::get_enthalpy(const messages::Transaction &transaction,
                       const messages::TaggedBlock &tagged_block,
                       const messages::Address &sender,
                       const messages::Address &recipient,
                       Double *enthalpy) const {
  Double amount = 0;
  for (const auto &input : transaction.inputs()) {
    messages::TaggedTransaction tagged_transaction;
    messages::Block input_block;
    bool include_transaction_pool = false;
    if (!_ledger->get_transaction(input.id(), &tagged_transaction, tagged_block,
                                  include_transaction_pool) ||
        !_ledger->get_block(tagged_transaction.block_id(), &input_block)) {
      return false;
    }
    for (const auto &output : tagged_transaction.transaction().outputs()) {
      if (output.address() == sender) {
        amount +=
            output.value().value() * (tagged_block.block().header().height() -
                                      input_block.header().height());
      }
    }
  }

  // TODO reference to a pdf in english that contains the formula
  Double previous_pii;
  _ledger->get_pii(sender, tagged_block.previous_assembly_id(), &previous_pii);
  *enthalpy =
      pow(mpfr::log(fmax(1, previous_pii * enthalpy_lambda() * enthalpy_c() *
                                amount / _config.blocks_per_assembly)),
          enthalpy_n());

  return true;
}

Config Pii::config() const { return _config; }

bool Pii::get_recipients(
    const messages::Transaction &transaction,
    std::unordered_set<messages::Address> *recipients) const {
  for (const auto &output : transaction.outputs()) {
    recipients->insert(output.address());
  }
  return true;
}

bool Pii::get_senders(const messages::Transaction &transaction,
                      const messages::TaggedBlock &tagged_block,
                      std::unordered_set<messages::Address> *senders) const {
  for (const auto &input : transaction.inputs()) {
    messages::TaggedTransaction tagged_transaction;
    bool include_transaction_pool = false;
    if (!_ledger->get_transaction(input.id(), &tagged_transaction, tagged_block,
                                  include_transaction_pool)) {
      return false;
    }
    if (tagged_transaction.transaction().outputs_size() <= input.output_id()) {
      return false;
    }
    senders->insert(
        tagged_transaction.transaction().outputs(input.output_id()).address());
  }
  return true;
}

bool Pii::add_transaction(const messages::Transaction &transaction,
                          const messages::TaggedBlock &tagged_block) {
  std::unordered_set<messages::Address> senders;
  std::unordered_set<messages::Address> recipients;
  if (!get_senders(transaction, tagged_block, &senders) ||
      !get_recipients(transaction, &recipients)) {
    return false;
  }
  for (const auto &sender : senders) {
    for (const auto &recipient : recipients) {
      Double enthalpy;
      if (!get_enthalpy(transaction, tagged_block, sender, recipient,
                        &enthalpy)) {
        return false;
      }
      if (sender != recipient) {
        _addresses.add_enthalpy(sender, recipient, enthalpy);
      }
    }
  }
  return true;
}

bool Pii::add_block(const messages::TaggedBlock &tagged_block) {
  // Warning: this method only works for blocks that are already inserted in the
  // ledger.
  // Notice that for now coinbases don't give any entropy
  for (const auto &transaction : tagged_block.block().transactions()) {
    if (!add_transaction(transaction, tagged_block)) {
      return false;
    }
  }
  return true;
}

void Addresses::add_enthalpy(const messages::Address &sender,
                             const messages::Address &recipient,
                             const Double &enthalpy) {
  Transactions *recipient_transactions = &(_addresses[recipient]);
  Counters *incoming = &(recipient_transactions->_in[sender]);
  incoming->nb_transactions++;
  incoming->enthalpy += enthalpy;

  Transactions *sender_transactions = &(_addresses[sender]);
  Counters *outgoing = &(sender_transactions->_out[recipient]);
  outgoing->nb_transactions += 1;
  outgoing->enthalpy += enthalpy;
}

Double Addresses::get_entropy(const messages::Address &address) const {
  Double entropy = 0;
  uint32_t total_nb_transactions = 0;
  auto transactions = &_addresses.at(address);
  for (const auto &[_, counters] : transactions->_in) {
    total_nb_transactions += counters.nb_transactions;
  }
  for (const auto &[_, counters] : transactions->_in) {
    Double p = (Double)counters.nb_transactions / (Double)total_nb_transactions;
    entropy -= counters.enthalpy * p * log2(p);
  }
  total_nb_transactions = 0;
  for (const auto &[_, counters] : transactions->_out) {
    total_nb_transactions += counters.nb_transactions;
  }
  for (const auto &[_, counters] : transactions->_out) {
    Double p = (Double)counters.nb_transactions / (Double)total_nb_transactions;
    entropy -= counters.enthalpy * p * log2(p);
  }
  return fmax(1, entropy);
}

std::vector<messages::Address> Addresses::addresses() const {
  std::vector<messages::Address> addresses;
  for (const auto &[address, _] : _addresses) {
    addresses.push_back(address);
  }
  return addresses;
}

std::vector<messages::Pii> Pii::get_addresses_pii(
    const messages::AssemblyHeight &assembly_height,
    const messages::BranchPath &branch_path) {
  std::vector<messages::Pii> piis;
  for (const auto &address : _addresses.addresses()) {
    auto &pii = piis.emplace_back();
    auto entropy = _addresses.get_entropy(address);

    // We want to get the integrity at the assembly n - 1
    // TODO add a reference to a translated version of Dhaou's paper
    auto integrity_score =
        _ledger->get_integrity(address, assembly_height - 1, branch_path);
    Double divided_integrity = integrity_score / 2400;
    auto integrity = 1 + 0.1 * divided_integrity / (1 + divided_integrity);
    auto score = fmax(1, integrity * entropy);
    pii.set_score(score.toString());
    pii.mutable_address()->CopyFrom(address);
  }
  std::sort(piis.begin(), piis.end(),
            [](const messages::Pii &a, const messages::Pii &b) {
              if (a.score() == b.score()) {
                return a.address().data() > b.address().data();
              }
              // Sorted in reverse order
              return a.score() > b.score();
            });
  return piis;
}

}  // namespace consensus
}  // namespace neuro
