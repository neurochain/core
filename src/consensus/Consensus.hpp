#ifndef NEURO_SRC_CONSENSUS_CONSENSUS_HPP
#define NEURO_SRC_CONSENSUS_CONSENSUS_HPP

#include <algorithm>
#include <random>
#include <unordered_set>
#include "consensus/Pii.hpp"
#include "ledger/Ledger.hpp"
#include "tooling/blockgen.hpp"

namespace neuro {
namespace consensus {
namespace tests {
class Consensus;
}

class Consensus {
 public:
  const Config config;

 private:
  std::shared_ptr<ledger::Ledger> _ledger;
  std::unordered_set<messages::Hash> _current_computations;
  std::unordered_map<messages::Hash, std::thread> _current_threads;
  std::mutex _current_computations_mutex;

  bool check_inputs(
      const messages::TaggedTransaction &tagged_transaction) const {
    const bool check_transaction_pool = tagged_transaction.has_block_id();

    messages::TaggedBlock tip;
    if (tagged_transaction.has_block_id()) {
      if (!_ledger->get_block(tagged_transaction.block_id(), &tip, false)) {
        LOG_INFO << "Could not find transaction block for transaction "
                 << tagged_transaction.transaction().id();
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
        LOG_INFO << "Failed check_input for transaction "
                 << tagged_transaction.transaction().id();
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
        LOG_INFO << "Failed check_output for transaction "
                 << tagged_transaction.transaction().id();
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
        LOG_INFO << "Failed check_output for transaction "
                 << tagged_transaction.transaction().id();
        return false;
      }
      if (input.output_id() >= input_transaction.transaction().outputs_size()) {
        LOG_INFO << "Failed check_output for transaction "
                 << tagged_transaction.transaction().id();
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

    bool result = total_spent == total_received;
    if (!result) {
      LOG_INFO << "Failed check_output for transaction "
               << tagged_transaction.transaction().id();
    }
    return result;
  }

  bool check_signatures(
      const messages::TaggedTransaction &tagged_transaction) const {
    bool result = crypto::verify(tagged_transaction.transaction());
    if (!result) {
      LOG_INFO << "Failed check_signatures for transaction "
               << tagged_transaction.transaction().id();
    }
    return result;
  }

  bool check_id(const messages::TaggedTransaction &tagged_transaction) const {
    auto transaction = messages::Transaction(tagged_transaction.transaction());
    messages::set_transaction_hash(&transaction);
    if (!tagged_transaction.transaction().has_id()) {
      LOG_INFO << "Failed check_id the transaction has no idea field";
    }
    bool result = transaction.id() == tagged_transaction.transaction().id();
    if (!result) {
      LOG_INFO << "Failed check_id for transaction "
               << tagged_transaction.transaction().id();
    }
    return result;
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
        LOG_INFO << "Failed check_double_inputs for transaction "
                 << tagged_transaction.transaction().id();
        LOG_DEBUG << "INPUTS" << tagged_transaction.transaction().inputs();
        throw std::runtime_error("toto");
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
      LOG_INFO << "Invalid coinbase for transaction "
               << tagged_transaction.transaction().id();
      return false;
    }

    // Check coinbase height
    if (!tagged_transaction.transaction().has_coinbase_height() ||
        tagged_transaction.transaction().coinbase_height() !=
            tagged_block.block().header().height()) {
      LOG_INFO << "Invalid coinbase for transaction "
               << tagged_transaction.transaction().id();
      return false;
    }

    // Check total spent
    if (tagged_block.block().header().height() != 0) {
      uint64_t to_spend =
          block_reward(tagged_block.block().header().height()).value();
      uint64_t total_spent = 0;
      for (const auto &output : tagged_transaction.transaction().outputs()) {
        total_spent += output.value().value();
      }
      if (to_spend != total_spent) {
        LOG_INFO << "Failed check_coinbase for transaction "
                 << tagged_transaction.transaction().id();
        return false;
      }
    }

    // Let's be strict here, a coinbase should not have any input or fee
    bool result = tagged_transaction.transaction().inputs_size() == 0 &&
                  !tagged_transaction.transaction().has_fees();
    if (!result) {
      LOG_INFO << "Failed check_coinbase for transaction "
               << tagged_transaction.transaction().id();
    }
    return result;
  }

  messages::NCCAmount block_reward(const messages::BlockHeight height) const {
    // TODO use some sort of formula here
    return config.block_reward;
  }

  bool check_block_id(messages::TaggedBlock *tagged_block) const {
    auto block = tagged_block->mutable_block();
    auto block_id = block->header().id();
    messages::set_block_hash(block);

    if (!block->header().has_id()) {
      LOG_INFO << "Failed check_block_id the block is missing the id field";
    }
    bool result = block->header().id() == block_id;
    if (!result) {
      // Try rehashing...
      auto new_block_id = block->header().id();
      messages::set_block_hash(block);
      assert(new_block_id == block->header().id());

      // Restore the original block_id
      block->mutable_header()->mutable_id()->CopyFrom(block_id);
    }

    if (!result) {
      LOG_INFO << "Failed check_block_id for block " << block->header().id();
    }
    return result;
  }

  bool check_block_transactions(
      const messages::TaggedBlock &tagged_block) const {
    const auto &block = tagged_block.block();
    if (block.coinbases_size() != 1) {
      LOG_INFO << "Failed check_block_transactions for block "
               << block.header().id();
      return false;
    }

    for (const auto transaction : block.coinbases()) {
      messages::TaggedTransaction tagged_transaction;
      tagged_transaction.set_is_coinbase(true);
      tagged_transaction.mutable_block_id()->CopyFrom(block.header().id());
      tagged_transaction.mutable_transaction()->CopyFrom(transaction);
      if (!is_valid(tagged_transaction)) {
        LOG_INFO << "Failed check_block_transactions for block "
                 << block.header().id();
        return false;
      }
    }

    for (const auto transaction : block.transactions()) {
      messages::TaggedTransaction tagged_transaction;
      tagged_transaction.set_is_coinbase(false);
      tagged_transaction.mutable_block_id()->CopyFrom(block.header().id());
      tagged_transaction.mutable_transaction()->CopyFrom(transaction);
      if (!is_valid(tagged_transaction)) {
        LOG_INFO << "Failed check_block_transactions for block "
                 << block.header().id();
        return false;
      }
    }

    return true;
  }

  bool check_block_size(const messages::TaggedBlock &tagged_block) const {
    const auto &block = tagged_block.block();
    Buffer buffer;
    messages::to_buffer(block, &buffer);
    bool result = buffer.size() < config.max_block_size;
    if (!result) {
      LOG_INFO << "Failed check_block_size for block " << block.header().id();
    }
    return result;
  }

  bool check_transactions_order(
      const messages::TaggedBlock &tagged_block) const {
    const auto &block = tagged_block.block();
    std::vector<std::string> transaction_ids;
    for (const auto &transaction : block.transactions()) {
      transaction_ids.push_back(messages::to_json(transaction.id()));
    }

    std::vector<std::string> orig_transaction_ids{transaction_ids};
    std::sort(transaction_ids.begin(), transaction_ids.end());

    bool result = orig_transaction_ids == transaction_ids;
    if (!result) {
      LOG_INFO << "Failed check_transactions_order for block "
               << block.header().id();
    }
    return result;
  }

  bool check_block_height(const messages::TaggedBlock &tagged_block) const {
    const auto &block = tagged_block.block();
    messages::Block previous;
    if (!_ledger->get_block(block.header().previous_block_hash(), &previous,
                            false)) {
      LOG_INFO << "Failed check_block_height for block " << block.header().id();
      return false;
    }

    return block.header().height() > previous.header().height();
  }

  bool check_block_timestamp(const messages::TaggedBlock &tagged_block) const {
    const auto &block = tagged_block.block();
    messages::Block block0;
    _ledger->get_block(0, &block0);
    auto expected_timestamp = block0.header().timestamp().data() +
                              block.header().height() * config.block_period;
    auto real_timestamp = tagged_block.block().header().timestamp().data();
    if (!block.header().has_timestamp()) {
      LOG_INFO << "Failed check_block_timestamp for block "
               << block.header().id() << " it is missing the timestamp field";
      return false;
    }
    if (real_timestamp < expected_timestamp) {
      LOG_INFO << "Failed check_block_timestamp for block "
               << block.header().id() << " it is too early" << std::endl
               << "Real timestamp " << real_timestamp << std::endl
               << "Expected timestamp " << expected_timestamp << std::endl;
      return false;
    }
    if (real_timestamp > expected_timestamp + config.block_period) {
      LOG_INFO << "Failed check_block_timestamp for block "
               << block.header().id() << " it is too late" << std::endl
               << "Real timestamp " << real_timestamp << std::endl
               << "Expected timestamp " << expected_timestamp << std::endl;
      return false;
    }
    return true;
  }

  bool check_block_author(const messages::TaggedBlock &tagged_block) const {
    messages::Assembly previous_assembly, previous_previous_assembly;
    const auto &block = tagged_block.block();

    if (!tagged_block.has_previous_assembly_id()) {
      LOG_INFO << "Failed check_block_author for block " << block.header().id()
               << " tagged block is missing field previous_assembly_id";
      return false;
    }
    if (!_ledger->get_assembly(tagged_block.previous_assembly_id(),
                               &previous_assembly)) {
      LOG_INFO << "Failed check_block_author for block " << block.header().id()
               << " failed to get previous assembly "
               << tagged_block.previous_assembly_id();
      return false;
    }
    if (!_ledger->get_assembly(previous_assembly.previous_assembly_id(),
                               &previous_previous_assembly)) {
      LOG_INFO << "Failed check_block_author for block " << block.header().id()
               << " failed to get previous previous assembly "
               << previous_assembly.previous_assembly_id();
      return false;
    }

    messages::Address address;
    if (!get_block_writer(previous_previous_assembly, block.header().height(),
                          &address)) {
      LOG_INFO << "Failed check_block_author for block " << block.header().id()
               << " could not find the block writer";
      return false;
    }

    const auto author_address = messages::Address(block.header().author());
    bool result = address == author_address;
    if (!result) {
      LOG_INFO << "Failed check_block_author for block " << block.header().id()
               << " the author address " << author_address
               << " does not match the required block writer " << address;
    }
    return result;
  }

  Double get_block_score(const messages::TaggedBlock &tagged_block) {
    messages::TaggedBlock previous;
    if (!_ledger->get_block(tagged_block.block().header().previous_block_hash(),
                            &previous) ||
        !previous.has_score()) {
      throw std::runtime_error(
          "Trying to score block " +
          messages::to_json(tagged_block.block().header().id()) +
          " when the previous block does not have a score");
    }
    return previous.score() + 1;
  }

  bool verify_blocks() {
    std::vector<messages::TaggedBlock> tagged_blocks;
    _ledger->get_unverified_blocks(&tagged_blocks);
    for (auto &tagged_block : tagged_blocks) {
      messages::TaggedBlock previous;
      if (!_ledger->get_block(
              tagged_block.block().header().previous_block_hash(), &previous,
              false)) {
        // Something is wrong here
        throw std::runtime_error(
            "Could not find the previous block of an unverified block " +
            messages::to_json(tagged_block.block().header().id()));
      }
      assert(previous.branch() == messages::Branch::MAIN ||
             previous.branch() == messages::Branch::FORK);
      bool added_new_assembly = false;
      if (is_new_assembly(tagged_block, previous)) {
        uint32_t first_height =
            previous.block().header().height() / config.blocks_per_assembly;
        uint32_t last_height = first_height + config.blocks_per_assembly - 1;
        _ledger->add_assembly(previous, first_height, last_height);
        added_new_assembly = true;
        tagged_block.mutable_previous_assembly_id()->CopyFrom(
            previous.block().header().id());

      } else if (!previous.has_previous_assembly_id()) {
        throw std::runtime_error(
            "A verified tagged_block is missing a previous_assembly_id " +
            messages::to_json(previous.block().header().id()));
      }
      auto assembly_id = added_new_assembly ? previous.block().header().id()
                                            : previous.previous_assembly_id();

      // Temporarily setup the previous_assembly_id so that check_block_author
      // can work
      tagged_block.mutable_previous_assembly_id()->CopyFrom(assembly_id);

      if (is_valid(&tagged_block)) {
        _ledger->set_block_verified(tagged_block.block().header().id(),
                                    get_block_score(tagged_block), assembly_id);
      } else if (!_ledger->delete_block_and_children(
                     tagged_block.block().header().id())) {
        throw std::runtime_error("Failed to delete an invalid block");
      } else {
        // The list of unverified blocks should have changed
        verify_blocks();
        return false;
      }
    }
    return true;
  }

  bool is_new_assembly(const messages::TaggedBlock &tagged_block,
                       const messages::TaggedBlock &previous) const {
    if (!previous.has_previous_assembly_id()) {
      throw std::runtime_error(
          "Something is wrong here block " +
          messages::to_json(previous.block().header().id()) +
          " is missing the previous_assembly_id");
    }
    int32_t block_assembly_height =
        tagged_block.block().header().height() / config.blocks_per_assembly;
    int32_t previous_assembly_height =
        previous.block().header().height() / config.blocks_per_assembly;
    return block_assembly_height != previous_assembly_height;
  }

 public:
  Consensus(std::shared_ptr<ledger::Ledger> ledger) : _ledger(ledger) {}
  Consensus(std::shared_ptr<ledger::Ledger> ledger, const Config &config)
      : config(config), _ledger(ledger) {}

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

  bool is_valid(messages::TaggedBlock *tagged_block) const {
    // This method should be only be called after the block has been inserted
    return check_block_id(tagged_block) &&
           check_block_transactions(*tagged_block) &&
           check_block_size(*tagged_block) &&
           check_transactions_order(*tagged_block) &&
           check_block_height(*tagged_block) &&
           check_block_timestamp(*tagged_block) &&
           check_block_author(*tagged_block);
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
    return _ledger->insert_block(block) && verify_blocks() &&
           _ledger->update_main_branch();
  }

  void add_wallet_key_pair(const std::shared_ptr<crypto::Ecc> wallet) {}

  void start_computations() {
    std::vector<messages::Assembly> assemblies;
    _ledger->get_assemblies_to_compute(&assemblies);
    for (const auto &assembly : assemblies) {
      if (_current_computations.count(assembly.id()) == 0) {
        if (add_current_computation(assembly.id())) {
          auto pii_thread =
              std::thread([&]() { compute_assembly_pii(assembly); });
          pii_thread.detach();
        }
      }
    }
  }

  bool add_current_computation(const messages::Hash &assembly_id) {
    std::lock_guard<std::mutex> lock(_current_computations_mutex);
    if (_current_computations.count(assembly_id) != 0) {
      // The computation was already added
      return false;
    }
    _current_computations.insert(assembly_id);
    return true;
  }

  bool remove_current_computation(const messages::Hash &assembly_id) {
    std::lock_guard<std::mutex> lock(_current_computations_mutex);
    if (_current_computations.count(assembly_id) == 0) {
      // The computation was already removed
      return false;
    }
    _current_computations.erase(assembly_id);
    return true;
  }

  void compute_assembly_pii(const messages::Assembly &assembly) {
    auto pii = Pii(_ledger, config);
    messages::TaggedBlock tagged_block;
    if (!_ledger->get_block(assembly.id(), &tagged_block)) {
      LOG_WARNING << "During Pii computation missing block " << assembly.id();
      remove_current_computation(assembly.id());
      return;
    }
    uint32_t seed = 0;

    while (tagged_block.block().header().id() !=
           assembly.previous_assembly_id()) {
      pii.add_block(tagged_block);
      auto block_id = tagged_block.block().header().id().data();
      seed += block_id.at(block_id.size()) % 2;
      seed <<= 1;
      if (!_ledger->get_block(
              tagged_block.block().header().previous_block_hash(),
              &tagged_block)) {
        LOG_WARNING << "During Pii computation missing block "
                    << tagged_block.block().header().previous_block_hash();
        remove_current_computation(assembly.id());
        return;
      }
    }

    auto piis = pii.get_addresses_pii();
    for (int i = 0; i < piis.size(); i++) {
      auto pii = piis[i];
      pii.mutable_assembly_id()->CopyFrom(assembly.id());
      pii.set_rank(i);
      _ledger->set_pii(pii);
    }
    _ledger->set_nb_addresses(assembly.id(), piis.size());

    remove_current_computation(assembly.id());
  }

  bool get_block_writer(const messages::Assembly &assembly,
                        const messages::BlockHeight &height,
                        messages::Address *address) const {
    if (!assembly.has_seed()) {
      LOG_WARNING << "Failed get_block_writer assembly is missing the seed";
      return false;
    }

    if (!assembly.has_nb_addresses()) {
      LOG_INFO << "Failed get_block_writer assembly is missing the number of "
                  "addresses";
      return false;
    }
    std::mt19937 rng;
    rng.seed(assembly.seed() + height);
    auto dist = std::uniform_int_distribution<std::mt19937::result_type>(
        0, std::min(assembly.nb_addresses(),
                    (int32_t)config.members_per_assembly) -
               1);
    auto writer_rank = dist(rng);
    if (!_ledger->get_block_writer(assembly.id(), writer_rank, address)) {
      LOG_WARNING << "Could not find block writer of rank " << writer_rank
                  << " in assembly " << assembly.id();
      return false;
    }
    return true;
  }

  messages::BlockHeight get_current_height() {
    messages::Block block0;
    _ledger->get_block(0, &block0);
    return (std::time(nullptr) - block0.header().timestamp().data()) /
           config.block_period;
  }

  bool heights_to_write(const messages::Address &address,
                        std::vector<messages::BlockHeight> *heights) {
    messages::TaggedBlock tagged_block;
    _ledger->get_block(_ledger->height(), &tagged_block);
    messages::Assembly previous_assembly, previous_previous_assembly;

    if (!_ledger->get_assembly(tagged_block.previous_assembly_id(),
                               &previous_assembly) ||
        !_ledger->get_assembly(previous_assembly.previous_assembly_id(),
                               &previous_previous_assembly)) {
      return false;
    }

    if (previous_previous_assembly.finished_computation()) {
      LOG_WARNING << "The computation of the assembly "
                  << previous_previous_assembly.id() << " is not finished.";
      return false;
    }
    auto height = get_current_height();
    for (auto i = height; i < previous_previous_assembly.last_height(); i++) {
      messages::Address writer;
      if (!get_block_writer(previous_previous_assembly, i, &writer)) {
        LOG_WARNING << "Did not manage to get the block writer for assembly "
                    << previous_previous_assembly.id() << " at height " << i;
      }
      if (writer == address) {
        heights->push_back(i);
      }
    }

    if (previous_assembly.finished_computation()) {
      for (auto i = previous_assembly.first_height();
           i < previous_assembly.last_height(); i++) {
        messages::Address writer;
        if (!get_block_writer(previous_assembly, i, &writer)) {
          LOG_WARNING << "Did not manage to get the block writer for assembly "
                      << previous_assembly.id() << " at height " << i;
        }
        if (writer == address) {
          heights->push_back(i);
        }
      }
    }

    return true;
  }

  bool write_block(const crypto::Ecc &keys, const messages::BlockHeight &height,
                   messages::Block *block) {
    messages::BlockHeader last_block_header;
    if (!_ledger->get_last_block_header(&last_block_header)) {
      return false;
    }
    auto header = block->mutable_header();
    keys.public_key().save(header->mutable_author());
    header->set_height(height);
    header->mutable_previous_block_hash()->CopyFrom(last_block_header.id());
    header->mutable_timestamp()->set_data(std::time(nullptr));

    // Block reward
    auto transaction = block->add_coinbases();
    tooling::blockgen::coinbase(keys.public_key(), block_reward(height),
                                transaction, height);

    // Transactions
    if (!_ledger->get_transaction_pool(block)) {
      return false;
    }

    messages::sort_transactions(block);
    messages::set_block_hash(block);

    // TODO sign blocks
    return true;
  }

  friend class neuro::consensus::tests::Consensus;
};  // namespace consensus

}  // namespace consensus
}  // namespace neuro

#endif /* NEURO_SRC_CONSENSUS_CONSENSUS_HPP */
