#ifndef NEURO_SRC_CONSENSUS_CONSENSUS_HPP
#define NEURO_SRC_CONSENSUS_CONSENSUS_HPP

#include <unordered_set>
#include "common/types.hpp"
#include "ledger/Ledger.hpp"
#include "messages/Message.hpp"
#include "networking/Networking.hpp"

namespace neuro {
namespace consensus {
namespace tests {
class Consensus;
}

const messages::NCCAmount BLOCK_REWARD{100};
const size_t MAX_BLOCK_SIZE{128000};

class Consensus {
 private:
  std::shared_ptr<ledger::Ledger> _ledger;
  std::unordered_set<messages::Hash> _current_computations;
  std::mutex _current_computations_mutex;

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

  bool check_double_inputs(
      const messages::TaggedTransaction &tagged_transaction) const {
    std::unordered_set<messages::Input> inputs;
    messages::Input clean_input;
    clean_input.set_signature_id(0);
    for (const auto &input : tagged_transaction.transaction().inputs()) {
      // We only want to compare the id and the output_id
      clean_input.mutable_id()->CopyFrom(input.id());
      clean_input.set_output_id(input.output_id());
      if (inputs.count(clean_input) > 0) {
        return false;
      }
      inputs.insert(clean_input);
    }
    return true;
  }

  bool check_coinbase(
      const messages::TaggedTransaction &tagged_transaction) const {
    messages::TaggedBlock tagged_block;
    if (!tagged_transaction.has_block_id() ||
        !_ledger->get_block(tagged_transaction.block_id(), &tagged_block,
                            false)) {
      return false;
    }

    // Check total spent
    if (tagged_block.block().header().height() != 0) {
      uint64_t to_spend = expected_block_reward(tagged_block).value();
      uint64_t total_spent = 0;
      for (const auto &output : tagged_transaction.transaction().outputs()) {
        total_spent += output.value().value();
      }
      if (to_spend != total_spent) {
        return false;
      }
    }

    // Let's be strict here, a coinbase should not have any input or fee
    return tagged_transaction.transaction().inputs_size() == 0 &&
           !tagged_transaction.transaction().has_fees();
  }

  messages::NCCAmount expected_block_reward(
      const messages::TaggedBlock tagged_block) const {
    // TODO use some sort of formula here
    return BLOCK_REWARD;
  }

  bool check_block_id(messages::Block *block) const {
    auto block_id = block->header().id();
    messages::set_block_hash(block);

    bool result = block->header().id() == block_id;
    if (!result) {
      // Try rehashing...
      auto new_block_id = block->header().id();
      messages::set_block_hash(block);
      assert(new_block_id == block->header().id());

      // Restore the original block_id
      block->mutable_header()->mutable_id()->CopyFrom(block_id);
    }

    return result;
  }

  bool check_block_transactions(const messages::Block &block) const {
    if (block.coinbases_size() != 1) {
      return false;
    }

    for (const auto transaction : block.coinbases()) {
      messages::TaggedTransaction tagged_transaction;
      tagged_transaction.set_is_coinbase(true);
      tagged_transaction.mutable_block_id()->CopyFrom(block.header().id());
      tagged_transaction.mutable_transaction()->CopyFrom(transaction);
      if (!is_valid(tagged_transaction)) {
        return false;
      }
    }

    for (const auto transaction : block.transactions()) {
      messages::TaggedTransaction tagged_transaction;
      tagged_transaction.set_is_coinbase(false);
      tagged_transaction.mutable_block_id()->CopyFrom(block.header().id());
      tagged_transaction.mutable_transaction()->CopyFrom(transaction);
      if (!is_valid(tagged_transaction)) {
        return false;
      }
    }

    return true;
  }

  bool check_block_size(const messages::Block &block) const {
    Buffer buffer;
    messages::to_buffer(block, &buffer);
    return buffer.size() < MAX_BLOCK_SIZE;
  }

  bool check_transactions_order(const messages::Block &block) const {
    std::vector<std::string> transaction_ids;
    for (const auto &transaction : block.transactions()) {
      transaction_ids.push_back(messages::to_json(transaction.id()));
    }

    std::vector<std::string> orig_transaction_ids{transaction_ids};
    std::sort(transaction_ids.begin(), transaction_ids.end());

    return orig_transaction_ids == transaction_ids;
  }

  bool check_block_height(const messages::Block &block) const {
    messages::Block previous;
    if (!_ledger->get_block(block.header().previous_block_hash(), &previous,
                            false)) {
      return false;
    }

    return block.header().height() > previous.header().height();
  }

  bool check_block_author(const messages::Block &block) const {
    // TODO
    return true;
  }

  Double get_block_score(const messages::TaggedBlock &tagged_block) {
    messages::TaggedBlock previous;
    if (!_ledger->get_block(tagged_block.block().header().previous_block_hash(),
                            &previous) ||
        !previous.has_score()) {
      return 0;
    }
    return previous.score() + 1;
  }

  bool verify_blocks() {
    std::vector<messages::TaggedBlock> tagged_blocks;
    _ledger->get_unverified_blocks(&tagged_blocks);
    for (auto &tagged_block : tagged_blocks) {
      if (is_valid(tagged_block.mutable_block())) {
        _ledger->set_block_verified(tagged_block.block().header().id(),
                                    get_block_score(tagged_block));
      } else {
        if (!_ledger->delete_block_and_children(
                tagged_block.block().header().id())) {
          return false;
        }
        // The list of unverified blocks should have changed
        return verify_blocks();
      }
    }
    return true;
  }

  bool is_new_assembly(const messages::TaggedBlock &tagged_block,
                       const messages::TaggedBlock &previous) const {
    int32_t block_assembly =
        tagged_block.block().header().height() / ASSEMBLY_BLOCKS_COUNT;
    int32_t previous_assembly =
        tagged_block.block().header().height() / ASSEMBLY_BLOCKS_COUNT;
    return block_assembly != previous_assembly;
  }

 public:
  Consensus(std::shared_ptr<ledger::Ledger> ledger)
      : _ledger(ledger), _current_computations() {}

  bool is_valid(const messages::TaggedTransaction &tagged_transaction) const {
    if (tagged_transaction.is_coinbase()) {
      return check_id(tagged_transaction) && check_coinbase(tagged_transaction);
    } else {
      return check_id(tagged_transaction) &&
             check_signatures(tagged_transaction) &&
             check_inputs(tagged_transaction) &&
             check_double_inputs(tagged_transaction) &&
             check_outputs(tagged_transaction);
    }
  }

  bool is_valid(messages::Block *block) const {
    // This method should be only be called after the block has been inserted
    return check_block_transactions(*block) && check_block_size(*block) &&
           check_block_id(block) && check_transactions_order(*block) &&
           check_block_author(*block) && check_block_height(*block);
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
  bool add_block(messages::Block *block) {
    if (!_ledger->insert_block(block)) {
      return false;
    }
    if (!verify_blocks()) {
      return false;
    }
    messages::TaggedBlock tagged_block, previous;
    if (!_ledger->get_block(block->header().id(), &tagged_block, false) ||
        !_ledger->get_block(block->header().previous_block_hash(), &previous,
                            false) ||
        tagged_block.branch() == messages::Branch::DETACHED) {
      // The block is detached
      return true;
    }
    assert(tagged_block.branch() == messages::Branch::MAIN ||
           tagged_block.branch() == messages::Branch::FORK);
    assert(previous.branch() == messages::Branch::MAIN ||
           previous.branch() == messages::Branch::FORK);
    if (is_new_assembly(tagged_block, previous)) {
      _ledger->add_assembly(previous);
      _ledger->set_previous_assembly_id(block->header().id(),
                                        previous.block().header().id());

    } else if (previous.has_previous_assembly_id()) {
      _ledger->set_previous_assembly_id(block->header().id(),
                                        previous.previous_assembly_id());
    } else {
      assert(previous.block().header().height() < ASSEMBLY_BLOCKS_COUNT);
    }
    return true;
  }

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

  void start_computations() {
    std::vector<messages::Assembly> assemblies;
    _ledger->get_assemblies_to_compute(&assemblies);
    for (const auto &assembly : assemblies) {
      if (_current_computations.count(assembly.assembly_id()) == 0) {
        if (add_current_computation(assembly.assembly_id())) {
          std::thread([&]() { compute_assembly_pii(assembly); });
        }
      }
    }
  }

  bool add_current_computation(const messages::Hash &assembly_id) {
    return true;
  }

  bool compute_assembly_pii(const messages::Assembly &assembly) { return true; }

  friend class neuro::consensus::tests::Consensus;
};

}  // namespace consensus
}  // namespace neuro

#endif /* NEURO_SRC_CONSENSUS_CONSENSUS_HPP */
