#ifndef PiiConsensus_H
#define PiiConsensus_H

#include <memory>

#include "common/types.hpp"
#include "Consensus.hpp"
#include "ForkManager.hpp"
#include "Pii.hpp"

namespace neuro {
namespace ledger {
class Ledger;
}  // ledger

namespace consensus {

/**
 *   \brief Consensus Calculate
 *   for each block add , the consensus check if is valide by testit,
 *
 */
class PiiConsensus : public Pii, public Consensus {
 private:
  std::shared_ptr<ledger::Ledger> _ledger;
  bool _valide_block;
  uint32_t _last_heigth_block;
  uint64_t _nonce_assembly = -1;
  int32_t _block_time = 15;

  ForkManager _ForkManager;

  void random_from_hashs();
  uint32_t ramdon_at(int index, uint64_t nonce) const;

 public:
  PiiConsensus(std::shared_ptr<ledger::Ledger> ledger) : PiiConsensus(ledger, 2000) {}
  PiiConsensus(std::shared_ptr<ledger::Ledger> ledger, uint32_t block_assembly);

  void add_block(const neuro::messages::Block &block);
  void add_blocks(const std::vector<neuro::messages::Block *> &blocks);
  Address get_next_owner() const;

  bool check_owner(const neuro::messages::BlockHeader &blockheader) const;
};

}  // namespace consensus
}  // namespace neuro
#endif  // PiiConsensus_H
