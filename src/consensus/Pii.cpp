#include "consensus/Pii.hpp"

namespace neuro {
namespace consensus {

bool Pii::add_transaction(const messages::Transaction &transaction) {
  // bool is_unspent_output(const messages::Transaction &transaction,
  //                        const int output_id) {
  return false;
}

bool Pii::add_block(const messages::Block &block) {
  for (const auto &transaction : block.transactions()) {
    add_transaction(transaction);
    const auto inputs = transaction.inputs();

    for (const auto &input : inputs) {
      const auto &key_pub = transaction.key_pubs(input.key_id());
      messages::Transaction prev_transaction;
      if (!_ledger->get_transaction(input.id(), &prev_transaction)) {
        LOG_ERROR << "Could not find previous transaction " << input.id();
        return false;
      }
    }

    // const auto outputs = transactions.outputs();
    // for (const auto &output : outputs) {
    // }
  }
}

}  // namespace consensus
}  // namespace neuro
