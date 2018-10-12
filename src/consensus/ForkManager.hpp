#ifndef NEURO_SRC_CONCENSUS_FORKMANAGER_HPP
#define NEURO_SRC_CONCENSUS_FORKMANAGER_HPP

#include "ForkTree.hpp"
#include "messages.pb.h"

namespace neuro {
namespace ledger {
class Ledger;
}  // namespace ledger

namespace consensus {

///
/// H1 : PII ✓
/// H2 : BLocks in Ledger ✓
/// H3 : Cancel BLock if prev in other block
///     ==> we have 8 block , 9 ==> 7 , Cancel 9
///
class ForkManager {
 public:
  enum class ForkStatus {
    Non_Fork = 0,  ///!< life is good
    Dual_Block,    ///!< the same owenr do 2 version of block
    VS_Block,  ///!< 1 of owner is fake by PII cal : the H1 is the caluclet of
               /// Pii is just
    Fork_Time,
    Olb_Block,
    Timeout_Block,
    Separate_Block,
  };

 protected:
 private:
 public:
  ForkManager();
  virtual ~ForkManager();

  ForkStatus fork_status(const messages::BlockHeader &blockheader,
                         const messages::BlockHeader &prev_blockheader,
                         const messages::BlockHeader &last_blockheader);

  void fork_results(ledger::Ledger *ledger);
};

}  // namespace consensus
}  // namespace neuro

#endif  // NEURO_SRC_CONCENSUS_FORKMANAGER_HPP!
