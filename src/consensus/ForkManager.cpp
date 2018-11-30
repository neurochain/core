#include "ForkManager.hpp"
#include "common/logger.hpp"
#include "ledger/Ledger.hpp"

namespace neuro {
namespace consensus {

ForkManager::ForkManager(std::shared_ptr<ledger::Ledger> ledger,
                         TransactionPool &transaction_pool)
    : _ledger(ledger), _transaction_pool(transaction_pool) {
  // ctor
}

ForkManager::~ForkManager() {
  // dto
}

ForkManager::ForkStatus ForkManager::fork_status(
    const messages::BlockHeader &blockheader,
    const messages::BlockHeader &prev_blockheader,
    const messages::BlockHeader &last_blockheader) {
  return ForkStatus::Non_Fork;
}

bool ForkManager::merge_fork_blocks() { return true; }

bool ForkManager::fork_results() {
  // TODO optimise this code and remove the return
  return false;
}

}  // namespace consensus
}  // namespace neuro
