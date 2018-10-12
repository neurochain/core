#ifndef Forkmanager_H
#define Forkmanager_H

#include "Forktree.hpp"
#include "messages.pb.h"
#include "ledger/LedgerMongodb.hpp"


namespace neuro {
namespace consensus {

///
/// H1 : PII ✓
/// H2 : BLocks in Ledger ✓
/// H3 : Cancel BLock if prev in other block
///     ==> we have 8 block , 9 ==> 7 , Cancel 9
///
class Forkmanager {
  public:
    enum ForkStatus {
        Non_Fork = 0, ///!< life is good
        Dual_Block, ///!< the same owenr do 2 version of block :)
        VS_Block,  ///!< 1 of owner is fake by PII cal : the H1 is the caluclet of Pii is just
        Fork_Time,
        Olb_Block,
        Timeout_Block,
        Separate_Block,
    };


  protected:
  private:

    public:

    Forkmanager();
    virtual ~Forkmanager();

    ForkStatus fork_status(const neuro::messages::BlockHeader &blockheader,
                           const neuro::messages::BlockHeader &prev_blockheader,
                           const neuro::messages::BlockHeader &last_blockheader);

    void fork_results( neuro::ledger::LedgerMongodb &ledger);
};

} // Consensus
} // Neuro

#endif // Forkmanager_H
