#include "ForkManager.hpp"
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
  int32_t blockHeight = blockheader.height(),
          prevHeight = prev_blockheader.height(),
          last_height = last_blockheader.height();
  ///
  /// Fork Generator
  ///

  ///!< the same height  (we have another block with the same height and the
  /// owner is valide see H1, H2
  ///!< set this in function for used for later see B2
  if (blockHeight == last_height) {
    ///!< the prev block hash is the same , impossible why
    if (prevHeight == blockHeight) {
      return ForkStatus::Dual_Block;
    }

    if (prevHeight == blockHeight - 1) {
      if (last_blockheader.author().SerializeAsString() ==
          blockheader.author().SerializeAsString()) {
        return ForkStatus::Dual_Block;
      } else {
        return ForkStatus::VS_Block;
      }
    }
  }

  ///!< B2 here
  if (blockHeight < last_height) {
    ///!< Old block or from other fork !!!* cancel see H3
    return ForkStatus::Olb_Block;
  }

  if (blockHeight > last_height + 1) {
    ///!< keep block ???? and valide owner
    if (prev_blockheader.has_height()) {  /// We have prev block
      if (prev_blockheader.height() != last_height) {
        ///!< used old block see H3
        return ForkStatus::Separate_Block;
      } else {
        return ForkStatus::Non_Fork;
      }
    } else {
      ///!< i'd have this fork H2
      return ForkStatus::Fork_Time;
    }
  }

  if (blockHeight == last_height + 1) {  ///!< nice one
    if (prev_blockheader.has_height()) {
      if (prevHeight != last_height) {
        ///!< cancel it  see H3
        return ForkStatus::Fork_Time;
      } else {
        return ForkStatus::Non_Fork;
      }
    } else {
      ///!< we don't have H2
      return ForkStatus::Fork_Time;
    }
  }

  return ForkStatus::Non_Fork;
}

bool ForkManager::fork_results() {
  //! load block 0
  messages::Block block0;
  _ledger->get_block(0, &block0);
  ForkTree forktree(block0, 0, ForkTree::MainBranch);
  uint64_t main_score = 0, fork_score = 0;
  //! Load all Main Block in trees
  int32_t h = _ledger->height();
  for (int32_t i = 1; i <= h; i++) {
    messages::Block block;
    if (_ledger->get_block(i, &block)) {
      if (!forktree.find_add(block, ForkTree::MainBranch, main_score)) {
        throw std::runtime_error(
            "Not found block in the main ledger");  // TO DO remove throw for
                                                    // best solutions
      }
    }
  }

  _ledger->fork_for_each([&](messages::Block &block) {
    if (!forktree.find_add(block, ForkTree::ForkBranch, fork_score)) {
      /*throw std::runtime_error(
          "Not found block ");  // TO DO remove throw for best solutions
        */
    }
  });

  if (fork_score > main_score) {
    forktree.update_branch();

    forktree.for_each([&](const messages::Hash &block_id,
                          ForkTree::BranchType old,
                          ForkTree::BranchType apply) {
      if (old != apply) {
        if (old == ForkTree::ForkBranch) {
          messages::Block block;
          _ledger->fork_get_block(block_id, &block);
          _ledger->fork_delete_block(block_id);
          _ledger->push_block(block);

          _transaction_pool.delete_transactions(block.transactions());

        } else {
          messages::Block block;
          _ledger->get_block(block_id, &block);
          _ledger->delete_block(block_id);
          _ledger->fork_add_block(block);

          _transaction_pool.add_transactions(block.transactions());
        }
      }
    });
    return true;
  }
  return false;
}

}  // namespace consensus
}  // namespace neuro
