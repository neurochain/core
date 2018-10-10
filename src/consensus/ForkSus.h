#ifndef FORKSUS_H
#define FORKSUS_H

#include "messages.pb.h"
#include "ledger/LedgerMongodb.hpp"


namespace neuro
{
namespace consensus
{

///
/// H1 : PII ✓
/// H2 : BLocks in Ledger ✓
/// H3 : Cancel BLock if prev in other block
///     ==> we have 8 block , 9 ==> 7 , Cancel 9
///
class ForkSus
{
public:
    enum ForkStatus
    {
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
    ForkSus();
    virtual ~ForkSus();

    ForkStatus fork_status(const neuro::messages::BlockHeader &bh,
                          const neuro::messages::BlockHeader &prev_bh,
                          const neuro::messages::BlockHeader &last_bh);
};

} // Consensus
} // Neuro

#endif // FORKSUS_H
