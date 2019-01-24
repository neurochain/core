#include "consensus/Pii.hpp"

namespace neuro {
namespace consensus {

Double Pii::enthalpy_n() const { return 1; }

Double Pii::enthalpy_c() const { return 1; }

Double Pii::enthalpy_lambda() const { return 0.5; }

bool Pii::get_enthalpy(const messages::Transaction &transaction,
                       const messages::TaggedBlock &tagged_block,
                       const messages::Address &sender,
                       const messages::Address &recipient,
                       Double *enthalpy) const {
  Double amount = 0;
  for (const auto &input : transaction.inputs()) {
    messages::TaggedTransaction tagged_transaction;
    messages::Block block;
    bool include_transaction_pool = false;
    if (!_ledger->get_transaction(input.id(), &tagged_transaction, tagged_block,
                                  include_transaction_pool) ||
        !_ledger->get_block(tagged_transaction.block_id(), &block)) {
      return false;
    }
    for (const auto &output : tagged_transaction.transaction().outputs()) {
      if (output.address() == sender) {
        amount +=
            output.value().value() *
            (tagged_block.block().header().height() - block.header().height());
      }
    }
  }

  // TODO reference to a pdf in english that contains the formula
  Double previous_pii;
  _ledger->get_pii(sender, tagged_block.previous_assembly_id(), &previous_pii);
  *enthalpy = std::pow(
      std::log(fmax(1, previous_pii * enthalpy_lambda() * enthalpy_c() *
                           amount / config.blocks_per_assembly)),
      enthalpy_n());
  return true;
}

bool Pii::get_recipients(const messages::Transaction &transaction,
                         std::vector<messages::Address> *recipients) const {
  for (const auto &output : transaction.outputs()) {
    recipients->push_back(output.address());
  }
  return true;
}

bool Pii::get_senders(const messages::Transaction &transaction,
                      const messages::TaggedBlock &tagged_block,
                      std::vector<messages::Address> *senders) const {
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
    senders->push_back(
        tagged_transaction.transaction().outputs(input.output_id()).address());
  }
  return true;
}

bool Pii::add_transaction(const messages::Transaction &transaction,
                          const messages::TaggedBlock &tagged_block) {
  std::vector<messages::Address> senders;
  std::vector<messages::Address> recipients;
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
      _addresses.add_enthalpy(sender, recipient, enthalpy);
    }
  }
  return true;
}

bool Pii::add_block(const messages::TaggedBlock &tagged_block) {
  // Warning: this method only works for blocks that are already inserted in the
  // ledger Notice that for now coinbases don't give any entropy
  for (const auto &transaction : tagged_block.block().transactions()) {
    if (!add_transaction(transaction, tagged_block)) {
      return false;
    }
  }
  return true;
}

void Addresses::add_enthalpy(const messages::Address &sender,
                             const messages::Address &recipient,
                             Double enthalpy) {
  Transactions *recipient_transactions =
      &(_addresses.try_emplace(recipient).first->second);
  Counters *incoming =
      &(recipient_transactions->_in.try_emplace(sender).first->second);
  incoming->nb_transactions += 1;
  incoming->enthalpy += enthalpy;

  Transactions *sender_transactions =
      &(_addresses.try_emplace(sender).first->second);
  Counters *outgoing =
      &(sender_transactions->_out.try_emplace(recipient).first->second);
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
  LOG_DEBUG << "TOTAL IN " << total_nb_transactions;
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

std::vector<messages::Pii> Pii::get_addresses_pii() {
  std::vector<messages::Pii> piis;
  for (const auto &[address, _] : _addresses._addresses) {
    auto &pii = piis.emplace_back();
    pii.set_score(_addresses.get_entropy(address));
    pii.mutable_address()->CopyFrom(address);
  }
  std::sort(piis.begin(), piis.end(),
            [](const messages::Pii &a, const messages::Pii &b) {
              return a.score() > b.score();  // Sorted in reverse order
            });
  return piis;
}

}  // namespace consensus
}  // namespace neuro
