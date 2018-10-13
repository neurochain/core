#ifndef NEURO_SRC_LEDGER_LEDGER_HPP
#define NEURO_SRC_LEDGER_LEDGER_HPP

#include <functional>
#include <optional>
#include "crypto/Hash.hpp"
#include "ledger/Filter.hpp"
#include "messages.pb.h"
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
    std::optional<const messages::Address> output() const { return _output; }
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
  virtual bool for_each(const Filter &filter, Functor functor) = 0;

  virtual bool fork_add_block(const messages::Block &b) = 0;
  virtual bool fork_delete_block(messages::Hash &id) = 0;
  virtual void fork_for_each(Functor_block functor) = 0;
  virtual void fork_test() = 0;
  virtual bool get_transaction(messages::Hash &id,
                               messages::Transaction *transaction) = 0;
  virtual bool add_transaction(const messages::Transaction &transaction) = 0;
  virtual bool delete_transaction(const messages::Hash &id) = 0;
  virtual int get_transaction_pool(messages::Block &block) = 0;

  // helpers

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
                         int output_id) {
    // TODO
    return true;
  }

  std::vector<messages::Output> get_outputs_for_address(
      const messages::Hasher &transaction_id,
      const messages::Address &address) {
    // TODO get transaction
    std::vector<messages::Output> result;
    messages::Transaction transaction;
    auto outputs = transaction.mutable_outputs();
    for (auto it(outputs->begin()); it != outputs->end(); it++) {
      if (it->address() == address) {
        it->set_output_id(std::distance(it, outputs->begin()));
        result.push_back(*it);
      }
    }
    return result;
  }

  virtual ~Ledger() {}
};

}  // namespace ledger
}  // namespace neuro

#endif /* NEURO_SRC_LEDGER_LEDGER_HPP */
