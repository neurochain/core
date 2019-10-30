#include <mpreal.h>
#include <cstdint>

#include "consensus/Pii.hpp"

namespace neuro {
namespace consensus {

Double Pii::enthalpy_n() const { return 1; }

Double Pii::enthalpy_c() const { return 1; }

Double Pii::enthalpy_lambda() const { return 0.5; }

Pii::~Pii() { mpfr_free_cache(); }

Config Pii::config() const { return _config; }

bool Pii::add_transaction(const messages::Transaction &transaction,
                          const messages::TaggedBlock &tagged_block,
                          const TotalSpent &total_spent,
                          const Balances &balances) {
  Double total_outputs = 0;
  for (const auto &output : transaction.outputs()) {
    total_outputs += output.value().value();
  }

  for (const auto &input : transaction.inputs()) {
    const auto spent_in_transaction = input.value().value();
    const auto spent_in_block = total_spent.at(input.key_pub());
    const Double input_ratio = Double{spent_in_transaction} / spent_in_block;
    const auto &balance = balances.at(input.key_pub());
    const Double enthalpy_spent_block =
        Double{balance.enthalpy_begin()} - balance.enthalpy_end();
    const Double enthalpy_spent_transaction =
        input_ratio * enthalpy_spent_block;
    Double previous_pii;
    _ledger->get_pii(input.key_pub(), tagged_block.previous_assembly_id(),
                     &previous_pii);
    for (const auto &output : transaction.outputs()) {
      if (output.key_pub() != input.key_pub()) {
        const Double output_ratio =
            Double{output.value().value()} / total_outputs;

        // We want the enthalpy in NCC and not in the smallest unit
        const Double raw_enthalpy =
            output_ratio * enthalpy_spent_transaction / 1E9;

        // TODO reference to a pdf in english that contains the formula
        const Double enthalpy =
            mpfr::pow(mpfr::log(mpfr::fmax(1, previous_pii * enthalpy_lambda() *
                                                  enthalpy_c() * raw_enthalpy /
                                                  _config.blocks_per_assembly)),
                      enthalpy_n());

        _key_pubs.add_enthalpy(input.key_pub(), output.key_pub(), enthalpy);
      }
    }
  }
  return true;
}

TotalSpent Pii::get_total_spent(const messages::Block &block) const {
  TotalSpent total_spent;
  for (const auto &transaction : block.transactions()) {
    for (const auto &input : transaction.inputs()) {
      total_spent[input.key_pub()] += input.value().value();
    }
  }
  return total_spent;
}

Balances Pii::get_balances(const messages::TaggedBlock &tagged_block) const {
  Balances balances;
  messages::TaggedBlock tagged_block_balances;
  _ledger->get_tagged_block_balances(tagged_block.block().header().id(),
                                     &tagged_block_balances);
  for (const auto &balance : tagged_block_balances.balances()) {
    balances[balance.key_pub()] = balance;
  }
  return balances;
}

bool Pii::add_block(const messages::TaggedBlock &tagged_block) {
  // Warning: this method only works for blocks that are already inserted in the
  // ledger.
  // Notice that for now coinbases don't give any entropy
  std::lock_guard lock(mpfr_mutex);  // TODO trax, is it really needed?
  auto total_spent = get_total_spent(tagged_block.block());
  auto balances = get_balances(tagged_block);

  for (const auto &transaction : tagged_block.block().transactions()) {
    if (!add_transaction(transaction, tagged_block, total_spent, balances)) {
      return false;
    }
  }
  return true;
}

void KeyPubs::add_enthalpy(const messages::_KeyPub &sender,
                           const messages::_KeyPub &recipient,
                           const Double &enthalpy) {
  Transactions *recipient_transactions = &(_key_pubs[recipient]);
  Counters *incoming = &(recipient_transactions->_in[sender]);
  incoming->nb_transactions++;
  incoming->enthalpy += enthalpy;

  Transactions *sender_transactions = &(_key_pubs[sender]);
  Counters *outgoing = &(sender_transactions->_out[recipient]);
  outgoing->nb_transactions += 1;
  outgoing->enthalpy += enthalpy;
}

Double KeyPubs::get_entropy(const messages::_KeyPub &key_pub) const {
  Double entropy = 0;
  uint32_t total_nb_transactions = 0;
  auto transactions = &_key_pubs.at(key_pub);
  for (const auto &[_, counters] : transactions->_in) {
    total_nb_transactions += counters.nb_transactions;
  }
  for (const auto &[_, counters] : transactions->_in) {
    Double p = Double{counters.nb_transactions} / total_nb_transactions;
    entropy -= counters.enthalpy * p * mpfr::log2(p);
  }
  total_nb_transactions = 0;
  for (const auto &[_, counters] : transactions->_out) {
    total_nb_transactions += counters.nb_transactions;
  }
  for (const auto &[_, counters] : transactions->_out) {
    Double p = Double{counters.nb_transactions} / total_nb_transactions;
    entropy -= counters.enthalpy * p * mpfr::log2(p);
  }
  return mpfr::fmax(1, entropy);
}

std::vector<messages::_KeyPub> KeyPubs::key_pubs() const {
  std::vector<messages::_KeyPub> key_pubs;
  for (const auto &[key_pub, _] : _key_pubs) {
    key_pubs.push_back(key_pub);
  }
  return key_pubs;
}

std::vector<messages::Pii> Pii::get_key_pubs_pii(
    const messages::AssemblyHeight &assembly_height,
    const messages::BranchPath &branch_path) {
  std::lock_guard lock(mpfr_mutex);  // TODO trax, is it really needed?
  std::vector<messages::Pii> piis;
  for (const auto &key_pub : _key_pubs.key_pubs()) {
    auto &pii = piis.emplace_back();
    auto entropy = _key_pubs.get_entropy(key_pub);

    // We want to get the integrity at the assembly n - 1
    // TODO add a reference to a translated version of Dhaou's paper
    Double integrity_score =
        _ledger->get_integrity(key_pub, assembly_height - 1, branch_path);
    Double divided_integrity = integrity_score / 2400;
    auto integrity = 1 + 0.1 * divided_integrity / (1 + divided_integrity);
    auto score = mpfr::fmax(1, integrity * entropy);
    pii.set_score(score.toString());
    pii.mutable_key_pub()->CopyFrom(key_pub);
  }
  std::sort(piis.begin(), piis.end(),
            [](const messages::Pii &a, const messages::Pii &b) {
              if (a.score() == b.score()) {
                return a.key_pub().raw_data() > b.key_pub().raw_data();
              }
              // Sorted in reverse order
              return a.score() > b.score();
            });
  return piis;
}

}  // namespace consensus
}  // namespace neuro
