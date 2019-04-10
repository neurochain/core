#ifndef NEURO_SRC_CONSENSUS_CONFIG_HPP
#define NEURO_SRC_CONSENSUS_CONFIG_HPP
#include <chrono>
#include "common/types.hpp"
#include "messages/Message.hpp"

namespace neuro {
namespace consensus {

struct Config {
  uint32_t blocks_per_assembly{5760};
  uint32_t members_per_assembly{500};
  uint32_t block_period{15};
  messages::NCCAmount block_reward{100};
  uint32_t max_block_size{128000};
  std::chrono::seconds update_heights_sleep{5};
  std::chrono::seconds compute_pii_sleep{5};
  std::chrono::milliseconds miner_sleep{100};
  int32_t integrity_block_reward{1};
  int32_t integrity_double_mining{-40};
  int32_t integrity_denunciation_reward{1};
};

}  // namespace consensus
}  // namespace neuro
#endif
