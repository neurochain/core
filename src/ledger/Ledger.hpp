#ifndef NEURO_SRC_LEDGER_LEDGER_HPP
#define NEURO_SRC_LEDGER_LEDGER_HPP

#include <functional>
#include <optional>
#include "crypto/Hash.hpp"
#include "crypto/Sign.hpp"
#include "ledger/Filter.hpp"
#include "messages.pb.h"
#include "messages/Message.hpp"

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
    std::optional<messages::_KeyPub> _output_key_pub;
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
  std::unordered_set<messages::Hash> _tips;
  
 public:
  Ledger() {}

  void new_tip(const messages::Hash &tip) {
    _tips.insert(tip);
  }

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
  virtual messages::TaggedBlocks get_blocks(
      const messages::BlockHeight height, const messages::_KeyPub &author,
      bool include_transactions = true) const = 0;
  virtual messages::TaggedBlocks get_blocks(const messages::Branch name) const = 0;
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
  virtual std::vector<messages::TaggedTransaction> get_transactions(
      const messages::TransactionID &id, const messages::TaggedBlock &tip,
      bool include_transaction_pool) const = 0;

  virtual bool add_transaction(
      const messages::TaggedTransaction &tagged_transaction) = 0;
  virtual bool add_to_transaction_pool(
      const messages::Transaction &transaction) = 0;
  virtual bool delete_transaction(const messages::TransactionID &id) = 0;
  virtual std::size_t get_transaction_pool(messages::Block *block) const = 0;
  virtual std::vector<messages::TaggedTransaction> get_transaction_pool()
      const = 0;

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

  virtual bool add_balances(messages::TaggedBlock *tagged_block) = 0;

  virtual bool cleanup_transactions(messages::Block *block) = 0;

  // helpers

  /*
   * List transactions for a key pub that are either in the main branch or in
   * the transaction pool
   */
  bool has_block(const messages::BlockID &id) const {
    messages::Block block;
    return get_block(id, &block);
  }

  messages::Transactions list_transactions(
      const messages::_KeyPub &key_pub) const {
    Filter filter;
    filter.output_key_pub(key_pub);
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
        it->set_output_id(std::distance(outputs->begin(), it));
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

  messages::NCCAmount balance(const messages::_KeyPub &key_pub) const {
    auto last_balance = get_balance(key_pub, get_main_branch_tip());
    return messages::NCCAmount(last_balance.value());
  }

  messages::Transaction build_transaction(
      const messages::_KeyPub &sender,
      const std::vector<messages::Output> &outputs,
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
    messages::Transaction transaction;

    auto outputs = std::vector<messages::Output>{1};
    auto output = &outputs[0];
    output->mutable_key_pub()->CopyFrom(recipient_key_pub);
    output->mutable_value()->set_value(amount.value());

    return build_transaction(sender_key_pub, outputs, fees);
  }

  messages::Transaction send_ncc(
      const crypto::KeyPriv &sender_key_priv,
      const messages::_KeyPub &recipient_key_pub, const float ratio_to_send,
      const std::optional<messages::NCCAmount> &fees = {}) const {
    assert(ratio_to_send <= 1);
    std::lock_guard<std::mutex> lock(_send_ncc_mutex);
    const auto sender_key_pub = sender_key_priv.make_key_pub();

    messages::NCCValue amount_to_send =
        balance(sender_key_pub).value() * ratio_to_send;

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
