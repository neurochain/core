#include "consensus/Pii.hpp"

namespace neuro {
namespace consensus {

bool Pii::add_block(const messages::TaggedBlock &tagged_block) {
  for (const auto &transaction : tagged_block.block().transactions()) {
    if (!_addresses.add_transaction(transaction, tagged_block)) {
      return false;
    }
  }
  for (const auto &transaction : tagged_block.block().coinbases()) {
    if (!_addresses.add_transaction(transaction, tagged_block)) {
      return false;
    }
  }
  return true;
}

bool Addresses::add_transaction(const messages::Transaction &transaction,
                                const messages::TaggedBlock &tagged_block) {
  for (const auto &output : transaction.outputs()) {
    const messages::Address address{output.address()};
    // if (_addresses.count(address) == 0) {
    //_addresses.insert({address, address});
    //}
    // std::unique_ptr<Transactions> transactions = _addresses.find(address);
    // transactions.add_incoming(transaction);
  }
  for (const auto &input : transaction.inputs()) {
    // TODO This is problematic I need the ledger here which is not available
  }
  return true;
}

}  // namespace consensus
}  // namespace neuro
