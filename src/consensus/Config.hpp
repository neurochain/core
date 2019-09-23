#ifndef NEURO_SRC_CONSENSUS_CONFIG_HPP
#define NEURO_SRC_CONSENSUS_CONFIG_HPP
#include <chrono>
#include "common/types.hpp"
#include "messages/Message.hpp"

namespace neuro {
namespace consensus {

struct Config {
  uint32_t blocks_per_assembly{200};
  uint32_t members_per_assembly{500};
  uint32_t block_period{3};
  messages::NCCAmount block_reward{uint64_t{100}};
  uint32_t max_block_size{MAX_MESSAGE_SIZE};
  std::chrono::seconds update_heights_sleep{5};
  std::chrono::seconds compute_pii_sleep{5};
  std::chrono::milliseconds miner_sleep{100};
  int32_t integrity_block_reward{1};
  int32_t integrity_double_mining{-40};
  int32_t integrity_denunciation_reward{1};
  uint32_t default_transaction_expires{5760};
};

}  // namespace consensus
}  // namespace neuro
#endif
