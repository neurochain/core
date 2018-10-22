#ifndef PiiConsensus_H
#define PiiConsensus_H

#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <memory>

#include "Consensus.hpp"
#include "ForkManager.hpp"
#include "Pii.hpp"
#include "TransactionPool.hpp"
#include "common/types.hpp"
#include "crypto/Ecc.hpp"
#include "networking/Networking.hpp"

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
  std::shared_ptr<networking::Networking> _network;
  std::map<const Address, std::shared_ptr<crypto::Ecc> > _wallets_keys;
  TransactionPool _transaction_pool;

  ForkManager _ForkManager;
  bool _valide_block;
  uint32_t _last_heigth_block;
  uint64_t _nonce_assembly = -1;

  void random_from_hashs();
  uint32_t ramdon_at(int index, uint64_t nonce) const;
  void init();

  bool block_in_ledger(const messages::Hash &id);
  boost::asio::steady_timer _timer_of_block_time;

  void timer_func();
  void ckeck_run_assembly(int32_t height);

 public:
  PiiConsensus(std::shared_ptr<boost::asio::io_context> io_context,
               std::shared_ptr<ledger::Ledger> ledger,
               std::shared_ptr<networking::Networking> network)
      : PiiConsensus(io_context, ledger, network, ASSEMBLY_BLOCKS_COUNT) {}

  PiiConsensus(std::shared_ptr<boost::asio::io_context> io_context,
               std::shared_ptr<ledger::Ledger> ledger,
               std::shared_ptr<networking::Networking> network,
               int32_t block_assembly);

  ~PiiConsensus() { _timer_of_block_time.cancel(); }

  int32_t next_height_by_time() const;
  void build_block();
  void add_transaction(const messages::Transaction &transaction);
  void add_block(const neuro::messages::Block &block, bool check_time = true);
  void add_blocks(const std::vector<neuro::messages::Block *> &blocks);
  Address get_next_owner() const;

  bool check_owner(const neuro::messages::BlockHeader &blockheader) const;
  std::string owner_at(int32_t index);

  void add_wallet_keys(const std::shared_ptr<crypto::Ecc> wallet);
};

}  // namespace consensus
}  // namespace neuro
#endif  // PiiConsensus_H
