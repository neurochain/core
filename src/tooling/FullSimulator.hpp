#ifndef NEURO_SRC_TOOLING_FULL_SIMULATOR_HPP
#define NEURO_SRC_TOOLING_FULL_SIMULATOR_HPP

#include <Bot.hpp>
#include "common/types.hpp"
#include "messages/Message.hpp"
#include "messages/config/Config.hpp"

namespace neuro {
namespace tooling {

const consensus::Config default_consensus_config{
    5,       // blocks_per_assembly
    10,      // members_per_assembly
    1,       // block_period
    messages::NCCAmount(100ull),  // block_reward
    128000,  // max_block_size
    1s,      // update_heights_sleep
    1s,      // compute_pii_sleep
    100ms,   // miner_sleep
    1,       // integrity_block_reward
    -40,     // integrity_double_mining
    1        // integrity_denunciation_reward
};

class FullSimulator {
 public:
  const std::string db_url;
  const std::string db_name;
  std::vector<std::unique_ptr<Bot>> bots;
  const float RATIO_TO_SEND = 0.5;
  std::vector<crypto::Ecc> keys;
  messages::Block block0;
  std::vector<messages::Address> addresses;
  std::unordered_map<messages::Address, uint32_t> addresses_indexes;

  messages::config::Config bot_config(const int bot_index, const int nb_bots,
                                      const double random_transaction) const;

  FullSimulator(
      const std::string &db_url, const std::string &db_name, const int nb_bots,
      const messages::NCCAmount ncc_block0,
      const consensus::Config &consensus_config = default_consensus_config,
      const double random_transaction = 0.5, const int32_t time_delta = 5);

  void send_random_transaction();
};

}  // namespace tooling
}  // namespace neuro

#endif
