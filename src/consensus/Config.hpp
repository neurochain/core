#ifndef NEURO_SRC_CONSENSUS_CONFIG_HPP
#define NEURO_SRC_CONSENSUS_CONFIG_HPP
#include "common/types.hpp"
#include "messages/Message.hpp"

namespace neuro {
namespace consensus {

struct Config {
  uint32_t blocks_per_assembly{12000};
  uint32_t members_per_assembly{500};
  uint32_t block_period{15};
  messages::NCCAmount block_reward{100};
  uint32_t max_block_size{128000};
};

}  // namespace consensus
}  // namespace neuro
#endif
