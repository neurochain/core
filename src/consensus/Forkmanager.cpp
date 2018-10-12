#include "Forkmanager.hpp"

namespace neuro {
namespace consensus {

Forkmanager::Forkmanager() {
    // ctor
}

Forkmanager::~Forkmanager() {
    // dto
}

Forkmanager::ForkStatus Forkmanager::fork_status(
    const neuro::messages::BlockHeader &blockheader,
    const neuro::messages::BlockHeader &prev_blockheader,
    const neuro::messages::BlockHeader &last_blockheader) {
    int32_t blockHeight = blockheader.height(), prevHeight = prev_blockheader.height(),
            last_height = last_blockheader.height();
    ///
    /// Fork Generator
    ///

    ///!< the same height  (we have another block with the same height and the
    ///owner is valide see H1, H2
    ///!< set this in function for used for later see B2
    if (blockHeight == last_height) {
        ///!< the prev block hash is the same , impossible why
        if (prevHeight == blockHeight) {
            throw std::runtime_error("the prev block hash is the same height");
        }

        if (prevHeight == blockHeight - 1) {
            if (last_blockheader.author().SerializeAsString() ==
                    blockheader.author().SerializeAsString()) {
                return ForkStatus::Dual_Block;
            } else {
                throw std::runtime_error("Fork with diff owner see H1");
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
        if (prev_blockheader.has_height()) { /// We have prev block
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

    if (blockHeight == last_height + 1) { ///!< nice one
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

    return Non_Fork;
}


void Forkmanager::fork_results( neuro::ledger::LedgerMongodb &ledger) {
       //! load block 0
       neuro::messages::Block block0;
       ledger.get_block(0, &block0);
       Forktree forktree(block0 ,0 , Forktree::MainBranch);

       //! Load all Main Block in trees
       int32_t h = ledger.height();
       for(int32_t i = 1; i<=h ; i++)
       {
            neuro::messages::Block block;
            ledger.get_block(i, &block);
            if ( !forktree.find_add(block, Forktree::MainBranch))
            {
                throw std::runtime_error("Not found block ");
            }
       }

       ledger.fork_for_each([&](neuro::messages::Block &block){
            if ( !forktree.find_add(block, Forktree::ForkBranch))
            {
                throw std::runtime_error("Not found block ");
            }
       });


       std::cout << forktree << std::endl;

}

}  // namespace consensus
}  // namespace neuro
