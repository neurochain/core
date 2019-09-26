#ifndef NEURO_SRC_TOOLING_FULL_SIMULATOR_HPP
#define NEURO_SRC_TOOLING_FULL_SIMULATOR_HPP

#include <Bot.hpp>
#include <cstdint>

#include "common/types.hpp"
#include "consensus/Config.hpp"
#include "messages.pb.h"
#include "messages/Message.hpp"
#include "messages/NCCAmount.hpp"
#include "messages/config/Config.hpp"

namespace neuro {
namespace tooling {

  static consensus::Config default_consensus_config{
						    .blocks_per_assembly = 5,
						    .members_per_assembly = 10,
						    .block_period = 1,
						    .block_reward = messages::NCCAmount{uint64_t{100}},
						    .max_block_size = 256000,
						    .max_transaction_per_block = 300,
						    .update_heights_sleep = 1s,
						    .compute_pii_sleep = 1s,
						    .miner_sleep = 100ms,
						    .integrity_block_reward = 1,
						    .integrity_double_mining = -40,
						    .integrity_denunciation_reward = 1,
						    .default_transaction_expires = 5760
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
