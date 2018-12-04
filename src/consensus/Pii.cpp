#include "consensus/Pii.hpp"

namespace neuro {
namespace consensus {

bool Transactions::add_block(const messages::Block &block) {
  for (const auto &transaction : block.transactions()) {
    auto emplaced_transaction = _transactions.emplace_back(transaction);
    TaggedTransaction tagger_transaction;
    
    for (const auto &input : inputs) {
      const auto &key_pub = transaction.key_pubs(input.key_id());
      if(!ledger->get_transaction(input.id(), &prev_transaction)) {
        LOG_ERROR << "Could not find previous transaction " << input.id();
        return false;
      }

      
    }

    for (const auto &output : outputs) {
      
    }
  }
}

}  // namespace consensus
}  // namespace neuro
