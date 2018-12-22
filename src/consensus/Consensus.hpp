#ifndef NEURO_SRC_CONSENSUS_CONSENSUS_HPP
#define NEURO_SRC_CONSENSUS_CONSENSUS_HPP

#include "ledger/Ledger.hpp"
#include "networking/Networking.hpp"

namespace neuro {
namespace consensus {
namespace tests {
class Consensus;
}

class Consensus {
 private:
  std::shared_ptr<ledger::Ledger> _ledger;

  bool check_inputs(
      const messages::TaggedTransaction &tagged_transaction) const {
    const bool check_transaction_pool = tagged_transaction.has_block_id();

    messages::TaggedBlock tip;
    if (tagged_transaction.has_block_id()) {
      if (!_ledger->get_block(tagged_transaction.block_id(), &tip, false)) {
        return false;
      };
    } else {
      tip = _ledger->get_main_branch_tip();
    }

    const auto transaction = tagged_transaction.transaction();
    for (const auto &input : transaction.inputs()) {
      ledger::Ledger::Filter filter;
      filter.input_transaction_id(input.id());
      filter.output_id(input.output_id());

      auto invalid = false;

      // Check that inputs are not spent by any other transaction
      _ledger->for_each(filter, tip, check_transaction_pool,
                        [&](const messages::TaggedTransaction match) {
                          if ((match.transaction().id() != transaction.id())) {
                            invalid = true;
                            return false;
                          }
                          return true;
                        });
      if (invalid) {
        return false;
      }
    }

    return true;
  }

  bool check_outputs(
      const messages::TaggedTransaction tagged_transaction) const {
    const bool check_transaction_pool = tagged_transaction.has_block_id();

    messages::TaggedBlock tip;
    if (tagged_transaction.has_block_id()) {
      if (!_ledger->get_block(tagged_transaction.block_id(), &tip, false)) {
        return false;
      };
    } else {
      tip = _ledger->get_main_branch_tip();
    }

    uint64_t total_received = 0;
    for (const auto &input : tagged_transaction.transaction().inputs()) {
      messages::TaggedTransaction input_transaction;
      if (!_ledger->get_transaction(input.id(), &input_transaction, tip,
                                    check_transaction_pool)) {
        // The input must exist
        return false;
      }
      if (input.output_id() >= input_transaction.transaction().outputs_size()) {
        return false;
      }
      total_received += input_transaction.transaction()
                            .outputs(input.output_id())
                            .value()
                            .value();
    }

    uint64_t total_spent = 0;
    for (const auto &output : tagged_transaction.transaction().outputs()) {
      total_spent += output.value().value();
    }
    if (tagged_transaction.transaction().has_fees()) {
      total_spent += tagged_transaction.transaction().fees().value();
    }

    LOG_INFO << "spent " << total_spent << " received " << total_received
             << std::endl;
    LOG_INFO << tagged_transaction;
    return total_spent == total_received;
  }

  bool check_signatures(
      const messages::TaggedTransaction &tagged_transaction) const {
    return crypto::verify(tagged_transaction.transaction());
  }

  bool check_id(const messages::TaggedTransaction &tagged_transaction) const {
    auto transaction = messages::Transaction(tagged_transaction.transaction());
    messages::set_transaction_hash(&transaction);
    return transaction.id() == tagged_transaction.transaction().id();
  }

 public:
  Consensus(std::shared_ptr<ledger::Ledger> ledger) : _ledger(ledger) {}

  void build_block() {}

  bool is_valid(const messages::TaggedTransaction &tagged_transaction) const {
    // Warning: this method only work for normal transactions and not coinbases
    return (
        check_id(tagged_transaction) && check_signatures(tagged_transaction) &&
        check_inputs(tagged_transaction) && check_outputs(tagged_transaction));
  }

  /**
   * \brief Add transaction to transaction pool
   * \param [in] transaction
   */
  bool add_transaction(const messages::Transaction &transaction) {
    return false;
  }

  /**
   * \brief Add block and compute Pii
   * \param [in] block block to add
   */
  bool add_block(const messages::Block &block) { return false; }

  // /**
  //  * \brief Get the next addr to build block
  //  * \return Address of next block builder
  //  */
  // messages::Address get_next_owner() const{}

  // /**
  //  * \brief Check if the owner of the block is the right one
  //  * \param [in] block_header the block header
  //  * \return false if the owner is not the right one
  //  */
  // bool check_owner(const messages::BlockHeader &block_header) const{}

  void add_wallet_key_pair(const std::shared_ptr<crypto::Ecc> wallet) {}

  friend class neuro::consensus::tests::Consensus;
};

}  // namespace consensus
}  // namespace neuro

#endif /* NEURO_SRC_CONSENSUS_CONSENSUS_HPP */
