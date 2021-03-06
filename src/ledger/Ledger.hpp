#ifndef NEURO_SRC_LEDGER_LEDGER_HPP
#define NEURO_SRC_LEDGER_LEDGER_HPP

#include <functional>
#include <optional>
#include "common/WorkerQueue.hpp"
#include "crypto/Hash.hpp"
#include "crypto/Sign.hpp"
#include "ledger/Filter.hpp"
#include "messages.pb.h"
#include "messages/Message.hpp"

namespace neuro {
namespace ledger {

template <class M>
class Cursor;

class Ledger {
 public:
  using Functor = std::function<bool(const messages::TaggedTransaction &)>;
  using BlocksIds = std::unordered_set<messages::BlockID>;
  struct MainBranchTip {
    messages::TaggedBlock main_branch_tip;
    bool updated;
  };

  // TODO use TransactionFilter from proto
  class Filter {
   private:
    std::optional<messages::_KeyPub> _output_key_pub;
    std::optional<messages::BlockID> _block_id;
    std::optional<messages::_KeyPub> _input_key_pub;
    std::optional<messages::TransactionID> _transaction_id;
    std::optional<std::size_t> _limit;
    std::optional<std::size_t> _skip;
    
   public:
    std::optional<std::size_t> limit() const { return _limit; }
    void limit(const std::size_t limit) { _limit = std::make_optional(limit); }
    
    std::optional<std::size_t> skip() const { return _skip; }
    void skip(const std::size_t skip) { _skip = std::make_optional(skip); }
    
    std::optional<const messages::_KeyPub> input_key_pub() const {
      return _input_key_pub;
    }

    void input_key_pub(const messages::_KeyPub &key_pub) {
      _input_key_pub = std::make_optional(key_pub);
    }

    void transaction_id(const messages::TransactionID &transaction_id) {
      _transaction_id =
          std::make_optional<messages::TransactionID>(transaction_id);
    }

    std::optional<const messages::TransactionID> transaction_id() const {
      return _transaction_id;
    }

    void output_key_pub(const messages::_KeyPub &key_pub) {
      _output_key_pub = std::make_optional<messages::_KeyPub>(key_pub);
    }
    const std::optional<messages::_KeyPub> output_key_pub() const {
      return _output_key_pub;
    }

    void block_id(const messages::BlockID &id) {
      _block_id = std::make_optional<messages::BlockID>(id);
    }
    std::optional<const messages::BlockID> block_id() const {
      return _block_id;
    }
  };

 private:
  mutable std::mutex _send_ncc_mutex;
  WorkerQueue<messages::TaggedBlock> _missing_blocks_queue;

  std::optional<messages::Hash> new_missing_block(
      const messages::TaggedBlock &tagged_block) {
    messages::TaggedBlock prev_tagged_block;
    if (tagged_block.branch() != messages::Branch::DETACHED) {
      std::lock_guard lock(_missing_block_mutex);
      _missing_blocks.erase(tagged_block.block().header().id());
      return std::nullopt;
    }

    if (_seen_blocks.count(tagged_block.block().header().id()) > 0) {
      return std::nullopt;
    }

    const auto &previous_block_hash =
        tagged_block.block().header().previous_block_hash();
    if (!get_block(previous_block_hash, &prev_tagged_block, false)) {
      if (previous_block_hash.has_data()) {
        std::lock_guard lock(_missing_block_mutex);
        _missing_blocks.insert(
            tagged_block.block().header().previous_block_hash());
        return previous_block_hash;
      }
      LOG_WARNING << "We probably have a DETACHED block0";
      return std::nullopt;
    }

    _missing_blocks_queue.push(
        std::make_shared<messages::TaggedBlock>(tagged_block));
    return std::nullopt;
  }

  void deep_missing_block(const messages::TaggedBlock &new_block) {
    messages::TaggedBlock tagged_block = new_block;
    messages::TaggedBlock prev_tagged_block;
    bool first_loop = true;
    while (true) {
      LOG_DEBUG << "deep_missing_block loop "
                << tagged_block.block().header().height();
      if (tagged_block.branch() != messages::Branch::DETACHED) {
        if (!first_loop) {
          // This is not normal the previous block should be attached
          LOG_DEBUG << "Calling set_branch_path from deep_missing_block";
          set_branch_path(prev_tagged_block.block().header());
        }
        std::lock_guard lock(_missing_block_mutex);
        _missing_blocks.erase(tagged_block.block().header().id());
        return;
      }

      if (_seen_blocks.count(tagged_block.block().header().id()) > 0) {
        return;
      }

      if (!get_block(tagged_block.block().header().previous_block_hash(),
                     &prev_tagged_block, false)) {
        std::lock_guard lock(_missing_block_mutex);
        _missing_blocks.insert(
            tagged_block.block().header().previous_block_hash());
        return;
      }

      _seen_blocks.insert(tagged_block.block().header().id());
      tagged_block.Swap(&prev_tagged_block);
      first_loop = false;
    }
  }

 protected:
  mutable std::recursive_mutex _missing_block_mutex;
  BlocksIds _missing_blocks;
  BlocksIds _seen_blocks;

 public:
  Ledger()
      : _missing_blocks_queue(
            [this](const messages::TaggedBlock &tagged_block) {
              this->deep_missing_block(tagged_block);
            }) {}

  const BlocksIds missing_blocks() {
    std::lock_guard lock(_missing_block_mutex);
    const auto copy = _missing_blocks;
    return copy;
  }

  std::optional<messages::Hash> new_missing_block(
      const messages::BlockID &block_id) {
    messages::TaggedBlock tagged_block;
    const bool block_found = get_block(block_id, &tagged_block, false);
    if (!block_found) {
      // this is weird, we should receive a block if it not inserted
      std::lock_guard lock(_missing_block_mutex);
      _missing_blocks.insert(block_id);
      return block_id;
    } else {
      std::lock_guard lock(_missing_block_mutex);
      _missing_blocks.erase(block_id);
      return new_missing_block(tagged_block);
    }
  }

  std::optional<messages::Hash> new_missing_block(
      const messages::World &world) {
    if (!world.has_missing_block()) {
      return std::nullopt;
    }
    messages::TaggedBlock tagged_block;

    if (!get_block(world.missing_block(), &tagged_block)) {
      std::lock_guard lock(_missing_block_mutex);
      _missing_blocks.insert(world.missing_block());
      return world.missing_block();
    }
    return new_missing_block(tagged_block);
  }

  virtual bool is_ancestor(const messages::BranchPath &ancestor_path,
                           const messages::BranchPath &block_path) const = 0;
  virtual bool is_ancestor(const messages::TaggedBlock &ancestor,
                           const messages::TaggedBlock &block) const = 0;
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
  virtual bool get_tagged_block_balances(
      const messages::BlockID &id,
      messages::TaggedBlock *tagged_block) const = 0;
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
  virtual messages::TaggedBlocks get_blocks(
      const messages::BlockHeight height, const messages::_KeyPub &author,
      bool include_transactions = true) const = 0;
  virtual messages::TaggedBlocks get_blocks(
      const messages::Branch name) const = 0;
  virtual bool insert_block(const messages::TaggedBlock &tagged_block) = 0;
  virtual bool insert_block(const messages::Block &block) = 0;
  virtual bool delete_block(const messages::BlockID &id) = 0;
  virtual bool delete_block_and_children(const messages::BlockID &id) = 0;
  virtual bool set_branch_invalid(const messages::BlockID &id) = 0;
  virtual bool for_each(const Filter &filter, Functor functor) const = 0;
  virtual bool for_each(const Filter &filter,
                        bool include_transaction_pool,
			const messages::TaggedBlock &tip,
			Functor functor) const = 0;
  virtual bool get_transaction(const messages::TransactionID &id,
                               messages::Transaction *transaction) const = 0;
  virtual bool get_transaction(const messages::TransactionID &id,
                               messages::Transaction *transaction,
                               messages::BlockHeight *blockheight) const = 0;
  virtual bool get_transaction(const messages::TransactionID &id,
                               messages::TaggedTransaction *tagged_transaction,
                               const messages::TaggedBlock &tip,
                               bool include_transaction_pool) const = 0;
  virtual std::vector<messages::TaggedTransaction> get_transactions(
      const messages::TransactionID &id, const messages::TaggedBlock &tip,
      bool include_transaction_pool) const = 0;

  virtual bool add_transaction(
      const messages::TaggedTransaction &tagged_transaction) = 0;
  virtual bool add_to_transaction_pool(
      const messages::Transaction &transaction) = 0;
  virtual bool delete_transaction(const messages::TransactionID &id) = 0;
  virtual Cursor<messages::TaggedTransaction> get_transaction_pool(
      const std::optional<std::size_t> size_limit = {}) const = 0;
  virtual std::size_t get_transaction_pool(
      messages::Block *block, const std::size_t size_limit,
      const std::size_t max_transactions) const = 0;

  // virtual bool get_blocks(int start, int size,
  // std::vector<messages::Block> &blocks) = 0;
  virtual std::size_t total_nb_transactions() const = 0;

  virtual std::size_t total_nb_blocks() const = 0;

  virtual messages::BranchPath fork_from(
      const messages::BranchPath &branch_path) const = 0;

  virtual messages::BranchPath first_child(
      const messages::BranchPath &branch_path) const = 0;

  virtual void empty_database() = 0;

  virtual void init_database(const messages::Block &block0) = 0;

  virtual Cursor<messages::TaggedBlock> get_unverified_blocks() const = 0;

  virtual bool set_block_verified(
      const messages::BlockID &id, const messages::BlockScore &score,
      const messages::AssemblyID previous_assembly_id) = 0;

  virtual bool update_main_branch() = 0;

  virtual bool get_assembly_piis(const messages::AssemblyID &assembly_id,
                                 std::vector<messages::Pii> *piis) = 0;

  virtual bool get_assembly(const messages::AssemblyID &assembly_id,
                            messages::Assembly *assembly) const = 0;

  virtual bool get_assembly(const messages::AssemblyHeight &height,
                            messages::Assembly *assembly) const = 0;

  virtual bool add_assembly(const messages::TaggedBlock &tagged_block,
                            const messages::AssemblyHeight height) = 0;

  virtual bool get_pii(const messages::_KeyPub &key_pub,
                       const messages::AssemblyID &assembly_id,
                       Double *score) const = 0;

  virtual bool set_pii(const messages::Pii &pii) = 0;

  virtual bool set_integrity(const messages::Integrity &integrity) = 0;

  virtual bool add_integrity(const messages::_KeyPub &key_pub,
                             const messages::AssemblyID &assembly_id,
                             const messages::AssemblyHeight &assembly_height,
                             const messages::BranchPath &branch_path,
                             const messages::IntegrityScore &added_score) = 0;

  virtual messages::IntegrityScore get_integrity(
      const messages::_KeyPub &key_pub,
      const messages::AssemblyHeight &assembly_height,
      const messages::BranchPath &branch_path) const = 0;

  virtual bool set_previous_assembly_id(
      const messages::BlockID &block_id,
      const messages::AssemblyID &previous_assembly_id) = 0;

  virtual bool get_next_assembly(const messages::AssemblyID &assembly_id,
                                 messages::Assembly *assembly) const = 0;

  virtual bool get_assemblies_to_compute(
      std::vector<messages::Assembly> *assemblies) const = 0;

  virtual bool get_block_writer(const messages::AssemblyID &assembly_id,
                                int32_t key_pub_rank,
                                messages::_KeyPub *key_pub) const = 0;

  virtual bool set_nb_key_pubs(const messages::AssemblyID &assembly_id,
                               int32_t nb_key_pubs) = 0;

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

  virtual messages::Balance get_balance(
      const messages::_KeyPub &key_pub,
      const messages::TaggedBlock &tagged_block) const = 0;

  virtual bool add_balances(messages::TaggedBlock *tagged_block,
                            int blocks_per_assembly) = 0;

  virtual int fill_block_transactions(messages::Block *block) const = 0;

  virtual bool set_branch_path(const messages::BlockHeader &block_header) = 0;

  // helpers

  /*
   * List transactions for a key pub that are either in the main branch or in
   * the transaction pool
   */
  bool has_block(const messages::BlockID &id) const {
    messages::Block block;
    return get_block(id, &block);
  }

  messages::Transactions list_transactions(const Filter filter) const {
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

  std::vector<messages::Output> get_outputs_for_key_pub(
      const messages::TransactionID &transaction_id,
      const messages::_KeyPub &key_pub) const {
    messages::Transaction transaction;
    get_transaction(transaction_id, &transaction);
    std::vector<messages::Output> result;
    auto outputs = transaction.mutable_outputs();
    for (auto it(outputs->begin()); it != outputs->end(); it++) {
      if (it->key_pub() == key_pub) {
        result.push_back(*it);
      }
    }
    return result;
  }

  bool has_received_transaction(const messages::_KeyPub &key_pub) const {
    Filter filter;
    filter.output_key_pub(key_pub);

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
      if (previous_hash.data().empty()) {
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

  messages::NCCAmount balance(const messages::_KeyPub &key_pub) const {
    auto last_balance = get_balance(key_pub, get_main_branch_tip());
    return messages::NCCAmount(last_balance.value());
  }

  template <class Outputs>
  messages::Transaction build_transaction(
      const messages::_KeyPub &sender, const Outputs &outputs,
      const std::optional<messages::NCCAmount> &fees = {}) const {
    messages::Transaction transaction;

    messages::NCCValue total_spent = 0;

    for (const auto &output : outputs) {
      transaction.add_outputs()->CopyFrom(output);
      total_spent += output.value().value();
    }

    if (fees) {
      transaction.mutable_fees()->CopyFrom(fees.value());
      total_spent += fees->value();
    }

    auto input = transaction.add_inputs();
    input->mutable_value()->set_value(total_spent);
    input->mutable_key_pub()->CopyFrom(sender);

    transaction.mutable_last_seen_block_id()->CopyFrom(
        get_main_branch_tip().block().header().id());

    return transaction;
  }

  messages::Transaction build_transaction(
      const messages::_KeyPub &sender_key_pub,
      const messages::_KeyPub &recipient_key_pub,
      const messages::NCCAmount &amount,
      const std::optional<messages::NCCAmount> &fees = {}) const {
    auto outputs = std::vector<messages::Output>{1};
    auto output = &outputs[0];
    output->mutable_key_pub()->CopyFrom(recipient_key_pub);
    output->mutable_value()->set_value(amount.value());

    return build_transaction(sender_key_pub, outputs, fees);
  }

  messages::Transaction send_ncc(
      const crypto::KeyPriv &sender_key_priv,
      const messages::_KeyPub &recipient_key_pub, const double ratio_to_send,

      const std::optional<messages::NCCAmount> &fees = {}) const {
    if (ratio_to_send < 0) {
      throw std::runtime_error("Cannot send_ncc with a ratio of " +
                               std::to_string(ratio_to_send));
    }
    const auto sender_key_pub = sender_key_priv.make_key_pub();
    auto amount_to_send =
        messages::NCCAmount(balance(sender_key_pub).value() * ratio_to_send);
    return send_ncc(sender_key_priv, recipient_key_pub, amount_to_send, fees);
  }

  messages::Transaction send_ncc(
      const crypto::KeyPriv &sender_key_priv,
      const messages::_KeyPub &recipient_key_pub,
      const messages::NCCAmount &amount_to_send,
      const std::optional<messages::NCCAmount> &fees = {}) const {
    std::lock_guard<std::mutex> lock(_send_ncc_mutex);
    const auto sender_key_pub = sender_key_priv.make_key_pub();

    auto transaction = build_transaction(sender_key_pub, recipient_key_pub,
                                         amount_to_send, fees);

    const auto ecc = crypto::Ecc(sender_key_priv, sender_key_pub);
    std::vector<const crypto::Ecc *> keys = {&ecc};

    crypto::sign(keys, &transaction);
    messages::set_transaction_hash(&transaction);
    return transaction;
  }

  virtual ~Ledger() {}
};

}  // namespace ledger
}  // namespace neuro

#endif /* NEURO_SRC_LEDGER_LEDGER_HPP */
