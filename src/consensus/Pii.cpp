#include "consensus/Pii.hpp"

namespace neuro {
namespace consensus {

bool Transactions::add_block(const messages::Block &block) {
  for (const auto &transaction : block.transactions()) {
    _transactions.emplace_back(transaction);
    for (const auto &input : inputs) {
      const auto &key_pub = transaction.key_pubs(input.key_id());
    }

    for (const auto &output : outputs) {
    }
  }
}

}  // namespace consensus
}  // namespace neuro
