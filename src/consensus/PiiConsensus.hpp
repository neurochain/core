#ifndef PiiConsensus_H
#define PiiConsensus_H

#include <memory>

#include "Consensus.hpp"
#include "ForkManager.hpp"
#include "Pii.hpp"
#include "common/types.hpp"
#include "crypto/Ecc.hpp"
#include "TransactionPool.hpp"

namespace neuro {
namespace ledger {
class Ledger;
}  // namespace ledger

namespace consensus {

/**
 *   \brief Consensus Calculate
 *   for each block add , the consensus check if is valide by testit,
 *
 */
class PiiConsensus : public Pii, public Consensus {
 private:
  std::shared_ptr<ledger::Ledger> _ledger;
  std::map<const Address , const crypto::Ecc *> _wallets_keys;
  TransactionPool _transaction_pool;

  ForkManager _ForkManager;
  bool _valide_block;
  uint32_t _last_heigth_block;
  uint64_t _nonce_assembly = -1;
  int32_t _block_time = 15;

  void random_from_hashs();
  uint32_t ramdon_at(int index, uint64_t nonce) const;
  void init();

 public:
  PiiConsensus(std::shared_ptr<ledger::Ledger> ledger)
      : PiiConsensus(ledger, 2000) {}

  PiiConsensus(std::shared_ptr<ledger::Ledger> ledger, uint32_t block_assembly);
  void add_transaction(const messages::Transaction &transaction);
  void add_block(const neuro::messages::Block &block);
  void add_blocks(const std::vector<neuro::messages::Block *> &blocks);
  Address get_next_owner() const;

  bool check_owner(const neuro::messages::BlockHeader &blockheader) const;
  void show_owner(uint32_t start, uint32_t how = 10);

  void add_wallet_keys(const crypto::Ecc *wallet);
};

}  // namespace consensus
}  // namespace neuro
#endif  // PiiConsensus_H
