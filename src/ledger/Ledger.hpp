#ifndef NEURO_SRC_LEDGER_LEDGER_HPP
#define NEURO_SRC_LEDGER_LEDGER_HPP

#include <functional>
#include <optional>
#include "crypto/Hash.hpp"
#include "crypto/Sign.hpp"
#include "ledger/Filter.hpp"
#include "messages.pb.h"
#include "messages/Address.hpp"
#include "messages/Message.hpp"
#include "rest.pb.h"

namespace neuro {
namespace ledger {

class Ledger {
 public:
  using Functor = std::function<bool(const messages::TaggedTransaction &)>;

  struct MainBranchTip {
    messages::TaggedBlock main_branch_tip;
    bool updated;
  };

  class Filter {
   private:
    std::optional<messages::Address> _output_address;
    std::optional<messages::BlockID> _block_id;
    std::optional<messages::TransactionID> _input_transaction_id;
    std::optional<messages::TransactionID> _transaction_id;
    std::optional<int32_t> _output_id;

   public:
    void input_transaction_id(const messages::TransactionID &transaction_id) {
      _input_transaction_id =
          std::make_optional<messages::TransactionID>(transaction_id);
    }

    std::optional<const messages::TransactionID> input_transaction_id() const {
      return _input_transaction_id;
    }

    void transaction_id(const messages::TransactionID &transaction_id) {
      _transaction_id =
          std::make_optional<messages::TransactionID>(transaction_id);
    }

    std::optional<const messages::TransactionID> transaction_id() const {
      return _transaction_id;
    }

    void output_id(const uint32_t outputid) {
      _output_id = std::make_optional<uint32_t>(outputid);
    }
    std::optional<const int32_t> output_id() const { return _output_id; }

    void output_address(const messages::Address &addr) {
      _output_address = std::make_optional<messages::Address>(addr);
    }
    const std::optional<messages::Address> output_address() const {
      return _output_address;
    }

    void block_id(const messages::BlockID &id) {
      _block_id = std::make_optional<messages::BlockID>(id);
    }
    std::optional<const messages::BlockID> block_id() const {
      return _block_id;
    }
  };

 private:
 public:
  Ledger() {}
  virtual messages::TaggedBlock get_main_branch_tip() const = 0;
  virtual bool set_main_branch_tip() = 0;
  virtual messages::BlockHeight height() const = 0;
  virtual bool get_block_header(const messages::BlockID &id,
                                messages::BlockHeader *header) const = 0;
  virtual bool get_last_block_header(
      messages::BlockHeader *block_header) const = 0;
  virtual bool get_last_block(messages::TaggedBlock *tagged_block,
                              bool include_transactions = true) const = 0;
  virtual bool get_block(const messages::BlockID &id, messages::Block *block,
                         bool include_transactions = true) const = 0;
  virtual bool get_block(const messages::BlockID &id,
                         messages::TaggedBlock *tagged_block,
                         bool include_transactions = true) const = 0;
  virtual bool get_block_by_previd(const messages::BlockID &previd,
                                   messages::Block *block,
                                   bool include_transactions = true) const = 0;
  virtual bool get_blocks_by_previd(
      const messages::BlockID &previd,
      std::vector<messages::TaggedBlock> *tagged_blocks,
      bool include_transactions = true) const = 0;
  virtual bool get_block(const messages::BlockHeight height,
                         messages::Block *block,
                         bool include_transactions = true) const = 0;
  virtual bool get_block(const messages::BlockHeight height,
                         messages::TaggedBlock *tagged_block,
                         bool include_transactions = true) const = 0;
  virtual bool get_block(const messages::BlockHeight height,
                         const messages::BranchPath &branch_path,
                         messages::TaggedBlock *tagged_block,
                         bool include_transactions = true) const = 0;
  virtual std::vector<messages::TaggedBlock> get_blocks(
      const messages::BlockHeight height, const messages::_KeyPub &author,
      bool include_transactions = true) const = 0;
  virtual bool insert_block(const messages::TaggedBlock &tagged_block) = 0;
  virtual bool insert_block(const messages::Block &block) = 0;
  virtual bool delete_block(const messages::BlockID &id) = 0;
  virtual bool delete_block_and_children(const messages::BlockID &id) = 0;
  virtual bool for_each(const Filter &filter, const messages::TaggedBlock &tip,
                        bool include_transaction_pool,
                        Functor functor) const = 0;
  virtual bool for_each(const Filter &filter, Functor functor) const = 0;
  virtual bool get_transaction(const messages::TransactionID &id,
                               messages::Transaction *transaction) const = 0;
  virtual bool get_transaction(const messages::TransactionID &id,
                               messages::Transaction *transaction,
                               messages::BlockHeight *blockheight) const = 0;
  virtual bool get_transaction(const messages::TransactionID &id,
                               messages::TaggedTransaction *tagged_transaction,
                               const messages::TaggedBlock &tip,
                               bool include_transaction_pool) const = 0;
  virtual bool add_transaction(
      const messages::TaggedTransaction &tagged_transaction) = 0;
  virtual bool add_to_transaction_pool(
      const messages::Transaction &transaction) = 0;
  virtual bool delete_transaction(const messages::TransactionID &id) = 0;
  virtual std::size_t get_transaction_pool(messages::Block *block) = 0;

  // virtual bool get_blocks(int start, int size,
  // std::vector<messages::Block> &blocks) = 0;
  virtual std::size_t total_nb_transactions() const = 0;

  virtual std::size_t total_nb_blocks() const = 0;

  virtual messages::BranchPath fork_from(
      const messages::BranchPath &branch_path) const = 0;

  virtual messages::BranchPath first_child(
      const messages::BranchPath &branch_path) const = 0;

  virtual void empty_database() = 0;

  virtual bool get_unverified_blocks(
      std::vector<messages::TaggedBlock> *tagged_blocks) const = 0;

  virtual bool set_block_verified(
      const messages::BlockID &id, const messages::BlockScore &score,
      const messages::AssemblyID previous_assembly_id) = 0;

  virtual bool update_main_branch() = 0;

  virtual bool get_assembly_piis(const messages::AssemblyID &assembly_id,
                                 std::vector<messages::Pii> *piis) = 0;

  virtual bool get_assembly(const messages::AssemblyID &assembly_id,
                            messages::Assembly *assembly) const = 0;

  virtual bool add_assembly(const messages::TaggedBlock &tagged_block,
                            const messages::AssemblyHeight height) = 0;

  virtual bool get_pii(const messages::Address &address,
                       const messages::AssemblyID &assembly_id,
                       Double *score) const = 0;

  virtual bool set_pii(const messages::Pii &pii) = 0;

  virtual bool set_integrity(const messages::Integrity &integrity) = 0;

  virtual bool add_integrity(const messages::Address &address,
                             const messages::AssemblyID &assembly_id,
                             const messages::AssemblyHeight &assembly_height,
                             const messages::BranchPath &branch_path,
                             const messages::IntegrityScore &added_score) = 0;

  virtual messages::IntegrityScore get_integrity(
      const messages::Address &address,
      const messages::AssemblyHeight &assembly_height,
      const messages::BranchPath &branch_path) const = 0;

  virtual bool set_previous_assembly_id(
      const messages::BlockID &block_id,
      const messages::AssemblyID &previous_assembly_id) = 0;

  virtual bool get_next_assembly(const messages::AssemblyID &assembly_id,
                                 messages::Assembly *assembly) const = 0;

  virtual bool get_assemblies_to_compute(
      std::vector<messages::Assembly> *assemblies) const = 0;

  virtual bool unsafe_get_assembly(const messages::AssemblyID &assembly_id,
                                   messages::Assembly *assembly) const = 0;

  virtual bool get_block_writer(const messages::AssemblyID &assembly_id,
                                int32_t address_rank,
                                messages::Address *address) const = 0;

  virtual bool set_nb_addresses(const messages::AssemblyID &assembly_id,
                                int32_t nb_addresses) = 0;

  virtual bool set_seed(const messages::AssemblyID &assembly_id,
                        int32_t seed) = 0;

  virtual bool set_finished_computation(
      const messages::AssemblyID &assembly_id) = 0;

  /*
   * Check if a denunciation exists in a certain branch within a block with a
   * block height at most max_block_height
   */
  virtual bool denunciation_exists(
      const messages::Denunciation &denunciation,
      const messages::BlockHeight &max_block_height,
      const messages::BranchPath &branch_path) const = 0;

  virtual void add_double_mining(
      const std::vector<messages::TaggedBlock> &tagged_blocks) = 0;

  virtual std::vector<messages::Denunciation> get_double_minings() const = 0;

  virtual void add_denunciations(
      messages::Block *block, const messages::BranchPath &branch_path,
      const std::vector<messages::Denunciation> &denunciations) const = 0;

  virtual void add_denunciations(
      messages::Block *block,
      const messages::BranchPath &branch_path) const = 0;

  // helpers

  /*
   * List transactions for an address that are either in the main branch or in
   * the transaction pool
   */
  bool has_block(const messages::BlockID &id) const {
    messages::Block block;
    return get_block(id, &block);
  }

  messages::Transactions list_transactions(
      const messages::Address &address) const {
    Filter filter;
    filter.output_address(address);
    messages::Transactions transactions;
    assert(get_main_branch_tip().branch() == messages::MAIN);

    for_each(
        filter,
        [&transactions](
            const messages::TaggedTransaction &tagged_transaction) -> bool {
          transactions.add_transactions()->CopyFrom(
              tagged_transaction.transaction());
          return true;
        });

    return transactions;
  }

  bool is_unspent_output(const messages::TransactionID &transaction_id,
                         int output_id) const {
    bool include_transaction_pool = true;
    return is_unspent_output(transaction_id, output_id, get_main_branch_tip(),
                             include_transaction_pool);
  }

  bool is_unspent_output(const messages::TransactionID &transaction_id,
                         int output_id, const messages::TaggedBlock &tip,
                         bool include_transaction_pool) const {
    Filter filter;
    filter.input_transaction_id(transaction_id);
    filter.output_id(output_id);

    bool match = false;
    for_each(filter, tip, include_transaction_pool,
             [&](const messages::TaggedTransaction &) {
               match = true;
               return false;
             });
    return !match;
  }

  std::vector<messages::Output> get_outputs_for_address(
      const messages::TransactionID &transaction_id,
      const messages::Address &address) const {
    messages::Transaction transaction;
    get_transaction(transaction_id, &transaction);
    std::vector<messages::Output> result;
    auto outputs = transaction.mutable_outputs();
    for (auto it(outputs->begin()); it != outputs->end(); it++) {
      if (it->address() == address) {
        it->set_output_id(std::distance(outputs->begin(), it));
        result.push_back(*it);
      }
    }
    return result;
  }

  bool has_received_transaction(const messages::Address &address) const {
    Filter filter;
    filter.output_address(address);

    bool match = false;
    for_each(filter, [&](const messages::TaggedTransaction &) {
      match = true;
      return false;
    });
    return match;
  }

  std::vector<messages::Block> get_last_blocks(
      const std::size_t nb_blocks) const {
    auto last_height = height();
    messages::Block block;
    get_block(last_height, &block);
    auto blocks = std::vector<messages::Block>{block};
    for (uint32_t i = 0; i < nb_blocks - 1; i++) {
      messages::BlockID previous_hash = block.header().previous_block_hash();
      if (previous_hash.data().size() == 0) {
        break;
      }
      if (get_block(previous_hash, &block)) {
        blocks.push_back(block);
      } else {
        return blocks;
      }
    }
    return blocks;
  }

  std::vector<messages::UnspentTransaction> list_unspent_transactions(
      const messages::Address &address) const {
    std::vector<messages::UnspentTransaction> unspent_transactions;

    auto transactions = list_transactions(address).transactions();
    for (const auto & transaction : transactions) {
      bool has_unspent_output = false;
      int64_t amount = 0;
      for (int i = 0; i < transaction.outputs_size(); i++) {
        const auto& output = transaction.outputs(i);
        if (output.address() == address &&
            is_unspent_output(transaction.id(), i)) {
          has_unspent_output = true;
          amount += output.value().value();
        }
      }
      if (has_unspent_output) {
        auto &unspent_transaction = unspent_transactions.emplace_back();
        unspent_transaction.mutable_transaction_id()->CopyFrom(
            transaction.id());
        unspent_transaction.mutable_value()->set_value(amount);
      }
    }
    return unspent_transactions;
  }

  /**
   * Return a list of transactions containing only the unspent outputs
   * for a given address
   */
  std::vector<messages::Transaction> list_unspent_outputs(
      const messages::Address &address) const {
    std::vector<messages::Transaction> result_transactions;

    auto transactions = list_transactions(address);
    for (auto &transaction : *transactions.mutable_transactions()) {
      std::vector<messages::Output> outputs;
      for (int i = 0; i < transaction.outputs_size(); i++) {
        auto output = transaction.outputs(i);
        if (output.address() == address &&
            is_unspent_output(transaction.id(), i)) {
          output.set_output_id(i);
          outputs.push_back(output);
        }
      }
      if (outputs.size() == 0) {
        continue;
      }
      transaction.mutable_outputs()->Clear();
      for (const auto output : outputs) {
        transaction.add_outputs()->CopyFrom(output);
      }
      result_transactions.push_back(transaction);
    }
    return result_transactions;
  }

  messages::NCCAmount balance(const messages::Address &address) {
    uint64_t total = 0;
    const auto unspent_transactions = list_unspent_transactions(address);
    for (const auto &unspent_transaction : unspent_transactions) {
      total += unspent_transaction.value().value();
    }
    return messages::NCCAmount(total);
  }

  void add_change(messages::Transaction *transaction,
                  const messages::Address &change_address,
                  uint64_t change_amount) const {
    if (change_amount) {
      auto change = transaction->add_outputs();
      change->mutable_value()->set_value(change_amount);
      change->mutable_address()->CopyFrom(change_address);
    }
  }

  void add_change(messages::Transaction *transaction,
                  const messages::Address &change_address) const {
    uint64_t inputs_ncc = 0;
    uint64_t outputs_ncc = 0;
    for (auto output : transaction->outputs()) {
      outputs_ncc += output.value().value();
    }
    if (transaction->has_fees()) {
      outputs_ncc += transaction->fees().value();
    }

    for (auto input : transaction->inputs()) {
      messages::Transaction transaction;
      get_transaction(input.id(), &transaction);
      auto output = transaction.outputs(input.output_id());
      inputs_ncc += output.value().value();
    }

    add_change(transaction, change_address, inputs_ncc - outputs_ncc);
  }

  std::vector<messages::Input> build_inputs(
      const std::vector<messages::TransactionID> &unspent_transactions_ids,
      const messages::Address &address) const {
    std::vector<messages::Input> inputs;

    // Process the outputs and lookup their output_id to build the inputs
    for (const auto &transaction_id : unspent_transactions_ids) {
      auto transaction_outputs =
          get_outputs_for_address(transaction_id, address);

      for (const auto &output : transaction_outputs) {
        auto &input = inputs.emplace_back();
        input.mutable_id()->CopyFrom(transaction_id);
        input.set_output_id(output.output_id());
        input.set_signature_id(0);
      }
    }
    return inputs;
  }

  /*
   * Build inputs for a new transaction given a list of transactions containing
   * only the unpsent output for the desired address.
   */
  std::vector<messages::Input> build_inputs(
      const std::vector<messages::Transaction> &unspent_outputs) const {
    std::vector<messages::Input> inputs;

    // The list of transactions contains only the unspent outputs which makes it
    // easier
    for (const auto &transaction : unspent_outputs) {
      for (const auto &output : transaction.outputs()) {
        auto &input = inputs.emplace_back();
        input.mutable_id()->CopyFrom(transaction.id());
        input.set_output_id(output.output_id());
        input.set_signature_id(0);
      }
    }
    return inputs;
  }

  messages::Transaction build_transaction(
      const std::vector<messages::TransactionID> &unspent_transactions_ids,
      const std::vector<messages::Output> &outputs,
      const crypto::KeyPriv &key_priv,
      const std::optional<const messages::NCCAmount> &fees = {}) const {
    const crypto::KeyPub key_pub = key_priv.make_key_pub();
    const auto address = messages::Address(key_pub);

    auto inputs = build_inputs(unspent_transactions_ids, address);

    return build_transaction(inputs, outputs, key_priv, fees);
  }

  messages::Transaction build_transaction(
      const std::vector<messages::Input> &inputs,
      const std::vector<messages::Output> &outputs,
      const crypto::KeyPriv &key_priv,
      const std::optional<messages::NCCAmount> &fees = {},
      bool add_change_output = true) const {
    messages::Transaction transaction;

    // Build keys
    const crypto::KeyPub key_pub = key_priv.make_key_pub();
    const auto address = messages::Address(key_pub);
    const auto ecc = crypto::Ecc(key_priv, key_pub);
    std::vector<const crypto::Ecc *> keys = {&ecc};

    for (auto input : inputs) {
      transaction.add_inputs()->CopyFrom(input);
    }

    for (auto output : outputs) {
      transaction.add_outputs()->CopyFrom(output);
    }

    if (fees) {
      transaction.mutable_fees()->CopyFrom(fees.value());
    }

    if (add_change_output) {
      add_change(&transaction, address);
    }

    // Sign the transaction
    crypto::sign(keys, &transaction);

    // Hash the transaction
    messages::set_transaction_hash(&transaction);

    return transaction;
  }

  messages::Transaction build_transaction(
      const messages::Address &address, const messages::NCCAmount &amount,
      const crypto::KeyPriv &key_priv,
      const std::optional<messages::NCCAmount> &fees = {}) const {
    auto bot_address = messages::Address(key_priv.make_key_pub());
    std::vector<messages::UnspentTransaction> unspent_transactions =
        list_unspent_transactions(bot_address);

    std::vector<messages::TransactionID> unspent_transactions_ids;
    for (const auto &unspent_transaction : unspent_transactions) {
      auto &unspent_transaction_id = unspent_transactions_ids.emplace_back();
      unspent_transaction_id.CopyFrom(unspent_transaction.transaction_id());
    }

    // Set outputs
    std::vector<messages::Output> outputs;
    auto &output = outputs.emplace_back();
    output.mutable_address()->CopyFrom(address);
    output.mutable_value()->CopyFrom(amount);
    return build_transaction(unspent_transactions_ids, outputs, key_priv, fees);
  }

  /*
   * Build a transaction using all the unspent outputs of a given address and
   * sending a ratio of all those coin to the recipient
   */
  messages::Transaction send_ncc(
      const crypto::KeyPriv &sender_key_priv,
      const messages::Address &recipient_address, const float ratio_to_send,
      const std::optional<messages::NCCAmount> &fees = {}) const {
    auto sender_address = messages::Address(sender_key_priv.make_key_pub());

    std::vector<messages::Transaction> unspent_outputs =
        list_unspent_outputs(sender_address);

    int32_t inputs_total = 0;
    for (const auto &transaction : unspent_outputs) {
      for (const auto output : transaction.outputs()) {
        inputs_total += output.value().value();
      }
    }

    // Set outputs
    std::vector<messages::Output> outputs;
    auto &output = outputs.emplace_back();
    output.mutable_address()->CopyFrom(recipient_address);
    uint64_t amount_to_send = (uint64_t)(inputs_total * ratio_to_send);
    output.mutable_value()->CopyFrom(messages::NCCAmount(amount_to_send));
    auto inputs = build_inputs(unspent_outputs);
    if (inputs_total - amount_to_send > 0) {
      auto &change = outputs.emplace_back();
      change.mutable_address()->CopyFrom(sender_address);
      change.mutable_value()->CopyFrom(
          messages::NCCAmount(inputs_total - amount_to_send));
    }

    bool add_change_output = false;

    auto result = build_transaction(inputs, outputs, sender_key_priv, fees,
                                    add_change_output);
    return result;
  }

  virtual ~Ledger() {}
};

}  // namespace ledger
}  // namespace neuro

#endif /* NEURO_SRC_LEDGER_LEDGER_HPP */
