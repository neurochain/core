#ifndef NEURO_SRC_LEDGER_LEDGER_HPP
#define NEURO_SRC_LEDGER_LEDGER_HPP

#include <functional>
#include <optional>
#include "crypto/Hash.hpp"
#include "crypto/Sign.hpp"
#include "ledger/Filter.hpp"
#include "messages.pb.h"
#include "messages/Address.hpp"
#include "rest.pb.h"

namespace neuro {
namespace ledger {

class Ledger {
 public:
  using Functor = std::function<bool(const messages::Transaction)>;
  using Functor_block = std::function<void(messages::Block &)>;

  class Filter {
   private:
    std::optional<messages::BlockHeight> _lower_height;
    std::optional<messages::BlockHeight> _upper_height;
    std::optional<messages::Address> _output;
    std::optional<messages::Hash> _block_id;

    std::optional<messages::Hash> _transaction_id;
    std::optional<int32_t> _output_id;

   public:
    void lower_bound(const messages::BlockHeight &height) {
      _lower_height = std::make_optional<messages::BlockHeight>(height);
    }

    void upper_bound(const messages::BlockHeight height) {
      _upper_height = std::make_optional<decltype(height)>(height);
    }

    void output_key_id(const messages::Address &output) {
      _output = std::make_optional<decltype(output)>(output);
    }

    void input_transaction_id(const messages::Hash &transaction_id) {
      _transaction_id = std::make_optional<messages::Hash>(transaction_id);
    }

    void output_id(const uint32_t outputid) {
      _output_id = std::make_optional<uint32_t>(outputid);
    }

    std::optional<const messages::BlockHeight> lower_height() const {
      return _lower_height;
    }
    std::optional<const messages::BlockHeight> upper_height() const {
      return _upper_height;
    }
    const std::optional<messages::Address> output() const { return _output; }
    std::optional<const messages::Hash> block_id() const { return _block_id; }

    std::optional<const messages::Hash> input_transaction_id() const {
      return _transaction_id;
    }
    std::optional<const int32_t> output_id() const { return _output_id; }
  };

 private:
 public:
  Ledger() {}
  virtual messages::BlockHeight height() const = 0;
  virtual bool get_block_header(const messages::BlockID &id,
                                messages::BlockHeader *header) = 0;
  virtual bool get_last_block_header(messages::BlockHeader *block_header) = 0;
  virtual bool get_block(const messages::BlockID &id,
                         messages::Block *block) = 0;
  virtual bool get_block(const messages::BlockHeight height,
                         messages::Block *block) = 0;

  virtual bool push_block(const messages::Block &block) = 0;
  virtual bool delete_block(const messages::Hash &id) = 0;
  virtual bool for_each(const Filter &filter, Functor functor) const = 0;

  virtual bool fork_add_block(const messages::Block &b) = 0;
  virtual bool fork_delete_block(const messages::Hash &id) = 0;
  virtual void fork_for_each(Functor_block functor) = 0;
  virtual bool fork_get_block(const messages::BlockID &id,
                              messages::Block *block) = 0;
  virtual bool fork_get_block(const messages::BlockHeight height,
                              messages::Block *block) = 0;
  virtual void fork_test() = 0;
  virtual bool get_transaction(const messages::Hash &id,
                               messages::Transaction *transaction) = 0;
  virtual bool add_transaction(const messages::Transaction &transaction) = 0;
  virtual bool delete_transaction(const messages::Hash &id) = 0;
  virtual int get_transaction_pool(messages::Block &block) = 0;

  virtual int total_nb_transactions() = 0;

  virtual int total_nb_blocks() = 0;

  // helpers

  messages::UnspentTransactions unspent_transactions(
      const messages::Address &address) {
    messages::UnspentTransactions unspent_transactions;

    const auto transactions = list_transactions(address).transactions();
    for (const auto &transaction : transactions) {
      for (int i = 0; i < transaction.outputs_size(); i++) {
        auto output = transaction.outputs(i);
        if (output.address() == address && is_unspent_output(transaction, i)) {
          auto unspent_transaction =
              unspent_transactions.add_unspent_transactions();
          unspent_transaction->set_transaction_id(transaction.id().data());
          unspent_transaction->set_value(
              std::to_string(output.value().value()));
        }
      }
    }

    return unspent_transactions;
  }

  messages::Transactions list_transactions(const messages::Address &address) {
    Filter filter;
    filter.output_key_id(address);
    messages::Transactions transactions;

    for_each(filter,
             [&transactions](const messages::Transaction &transaction) -> bool {
               transactions.add_transactions()->CopyFrom(transaction);
               return true;
             });

    return transactions;
  }

  bool is_unspent_output(const messages::Transaction &transaction,
                         const int output_id) {
    Filter filter;
    filter.input_transaction_id(transaction.id());
    filter.output_id(output_id);

    bool match = false;
    for_each(filter, [&](const messages::Transaction) {
      match = true;
      return false;
    });
    return !match;
  }

  std::vector<messages::Output> get_outputs_for_address(
      const messages::Hasher &transaction_id,
      const messages::Address &address) {
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

  bool has_received_transaction(const messages::Address &address) {
    Filter filter;
    filter.output_key_id(address);

    bool match = false;
    for_each(filter, [&](const messages::Transaction _) {
      match = true;
      return false;
    });
    return match;
  }

  std::vector<messages::Block> get_last_blocks(const uint32_t nb_blocks) {
    auto last_height = height();
    messages::Block block;
    get_block(last_height, &block);
    auto blocks = std::vector<messages::Block>{block};
    for (uint32_t i = 0; i < nb_blocks - 1; i++) {
      messages::Hash previous_hash = block.header().previous_block_hash();
      if (previous_hash.data().size() == 0) {
        break;
      }
      get_block(previous_hash, &block);
      blocks.push_back(block);
    }
    return blocks;
  }

  messages::Transaction build_transaction(
      const messages::TransactionToPublish &transaction_to_publish) {
    messages::Transaction transaction;

    // Load keys
    auto buffer = Buffer(transaction_to_publish.key_priv());
    const auto random_pool = std::make_shared<CryptoPP::AutoSeededRandomPool>();
    auto key_priv = crypto::EccPriv(random_pool);
    key_priv.load(buffer);
    const crypto::EccPub key_pub = key_priv.make_public_key();
    const auto address = messages::Address(key_pub);
    const auto ecc = crypto::Ecc(key_priv, key_pub);
    std::vector<const crypto::Ecc *> keys = {&ecc};

    uint64_t inputs_ncc = 0;
    uint64_t outputs_ncc = 0;
    auto outputs_to_publish = transaction_to_publish.outputs();
    for (auto output : outputs_to_publish) {
      outputs_ncc += output.value().value();
    }

    // Process the outputs and lookup their output_id to build the inputs
    auto transaction_ids = transaction_to_publish.transactions_ids();
    for (auto transaction_id_str : transaction_ids) {
      messages::Hasher transaction_id;
      transaction_id.set_type(messages::Hash_Type_SHA256);
      transaction_id.set_data(transaction_id_str);
      auto outputs = get_outputs_for_address(transaction_id, address);
      for (auto output : outputs) {
        auto input = transaction.add_inputs();
        input->mutable_id()->CopyFrom(transaction_id);
        input->set_output_id(output.output_id());
        input->set_key_id(0);
        inputs_ncc += output.value().value();
      }
    }

    transaction.mutable_outputs()->CopyFrom(transaction_to_publish.outputs());

    // Add change output
    if (outputs_ncc < inputs_ncc) {
      auto change = transaction.add_outputs();
      change->mutable_value()->set_value(inputs_ncc - outputs_ncc);
      change->mutable_address()->CopyFrom(address);
    }

    transaction.mutable_fees()->CopyFrom(transaction_to_publish.fees());

    // Sign the transaction
    crypto::sign(keys, &transaction);

    // Hash the transaction
    messages::hash_transaction(&transaction);

    return transaction;
  }

  virtual ~Ledger() {}
};

}  // namespace ledger
}  // namespace neuro

#endif /* NEURO_SRC_LEDGER_LEDGER_HPP */
