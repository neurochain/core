#ifndef NEURO_SRC_TOOLING_REALTIMESIMULATOR_HPP
#define NEURO_SRC_TOOLING_REALTIMESIMULATOR_HPP

#include "consensus/Consensus.hpp"
#include "crypto/Ecc.hpp"
#include "ledger/LedgerMongodb.hpp"

namespace neuro {
namespace tooling {

consensus::Config realtime_config{
    10,     // blocks_per_assembly
    10,     // members_per_assembly
    3,      // block_period
    100,    // block_reward
    128000  // max_block_size
};

class RealtimeSimulator {
 private:
  messages::NCCAmount _ncc_block0;

 public:
  std::vector<crypto::Ecc> keys;
  std::shared_ptr<neuro::ledger::LedgerMongodb> ledger;
  std::shared_ptr<neuro::consensus::Consensus> consensus;
  RealtimeSimulator(const std::string &db_url, const std::string &db_name,
                    const messages::NCCAmount ncc_block0)
      : _ncc_block0(ncc_block0),
        keys(1),
        ledger(std::make_shared<ledger::LedgerMongodb>(
            db_url, db_name,
            tooling::blockgen::gen_block0(keys, _ncc_block0, 0).block())),
        consensus(std::make_shared<consensus::Consensus>(
            ledger, std::make_shared<crypto::Ecc>(keys[0]), realtime_config,
            consensus::PublishBlock())) {}
};

}  // namespace tooling
}  // namespace neuro

#endif
