#ifndef NEURO_SRC_LEDGER_LEDGER_HPP
#define NEURO_SRC_LEDGER_LEDGER_HPP

#include <functional>
#include <optional>
#include "ledger/Filter.hpp"
#include "messages.pb.h"
#include "crypto/Hash.hpp"

namespace neuro {
namespace ledger {

class Ledger {
 public:
  using Functor = std::function<bool(const messages::Transaction)>;
  class Filter {
   private:
    std::optional<messages::BlockHeight> _lower_height;
    std::optional<messages::BlockHeight> _upper_height;
    std::optional<messages::Address> _output;
    std::optional<messages::Hash> _block_id;

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

    std::optional<const messages::BlockHeight> lower_height() const {
      return _lower_height;
    }
    std::optional<const messages::BlockHeight> upper_height() const {
      return _upper_height;
    }
    std::optional<const messages::Address> output() const { return _output; }
    std::optional<const messages::Hash> block_id() const { return _block_id; }
  };

 private:
 public:
  virtual messages::BlockHeight height() const = 0;
  virtual bool get_block_header(const messages::BlockID &id,
                                messages::BlockHeader *header) = 0;
  virtual bool get_last_block_header(messages::BlockHeader *block_header) = 0;
  virtual bool get_block(const messages::BlockID &id,
                         messages::Block *block) = 0;
  virtual bool get_block(const messages::BlockHeight height,
                         messages::Block *block) = 0;
  virtual bool push_block(const messages::Block &block) = 0;
  virtual bool for_each(const Filter &filter, Functor functor) = 0;

  // helpers
  
  messages::Transactions list_transactions (const messages::Address &address) {
    Filter filter;
    filter.output_key_id(address);
    messages::Transactions transactions;
    
    for_each (filter,
	      [&transactions] (const messages::Transaction &transaction) -> bool {
		transactions.add_transactions()->CopyFrom(transaction);
		return true;
	      });

    return transactions;
  }
  
  virtual ~Ledger() {}
};

}  // namespace ledger
}  // namespace neuro

#endif /* NEURO_SRC_LEDGER_LEDGER_HPP */
