#ifndef NEURO_SRC_TOOLING_FULL_SIMULATOR_HPP
#define NEURO_SRC_TOOLING_FULL_SIMULATOR_HPP

#include <Bot.hpp>
#include "common/types.hpp"
#include "messages/Message.hpp"
#include "messages/config/Config.hpp"

namespace neuro {
namespace tooling {

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

  messages::config::Config bot_config(const int bot_index,
                                      const int nb_bots) const;

  FullSimulator(const std::string &db_url, const std::string &db_name,
                const int nb_bots, const messages::NCCAmount ncc_block0,
                const int32_t time_delta = 5);
};

}  // namespace tooling
}  // namespace neuro

#endif
