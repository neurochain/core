#ifndef NEURO_SRC_TOOLING_FULL_SIMULATOR_HPP
#define NEURO_SRC_TOOLING_FULL_SIMULATOR_HPP

#include <Bot.hpp>
#include "common/types.hpp"
#include "messages/Message.hpp"
#include "messages/config/Config.hpp"

namespace neuro {
namespace tooling {

consensus::Config consensus_config{
    5,       // blocks_per_assembly
    10,      // members_per_assembly
    1,       // block_period
    100,     // block_reward
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
  std::vector<messages::Address> addresses;
  std::unordered_map<messages::Address, uint32_t> addresses_indexes;

  messages::config::Config bot_config(const int bot_index,
                                      const int nb_bots) const {
    messages::config::Config config;
    auto networking = config.mutable_networking();
    networking->set_max_connections(3);
    auto tcp = networking->mutable_tcp();
    tcp->set_port(1337 + bot_index);
    tcp->set_endpoint("localhost");
    for (int j = 0; j < nb_bots; j++) {
      if (j != bot_index) {
        auto peer = tcp->add_peers();
        peer->set_endpoint("localhost");
        peer->set_port(1337 + j);
        peer->mutable_key_pub()->CopyFrom(keys[j].key_pub());
      }
    }
    auto new_keys = networking->add_keys();
    new_keys->mutable_key_pub()->CopyFrom(keys[bot_index].key_pub());
    new_keys->mutable_key_priv()->CopyFrom(keys[bot_index].key_priv());
    auto database = config.mutable_database();
    database->set_url(db_url);
    database->set_db_name(db_name + std::to_string(bot_index));
    database->set_block0_format(messages::config::Database::PROTO);
    database->set_block0_path("data.0.testnet");
    return config;
  }

  FullSimulator(const std::string &db_url, const std::string &db_name,
                const int nb_bots, const messages::NCCAmount ncc_block0,
                const int32_t time_delta = 5)
      : db_url(db_url), db_name(db_name), keys(nb_bots) {
    // time_delta puts the block0 n seconds in the future so that we have the
    // time to initialize the bots
    auto block0 =
        tooling::blockgen::gen_block0(keys, ncc_block0, time_delta).block();
    for (size_t i = 0; i < keys.size(); i++) {
      auto &address = addresses.emplace_back(keys[i].key_pub());
      addresses_indexes.insert({address, i});
    }
    for (int i = 0; i < nb_bots; i++) {
      LOG_DEBUG << "CREATING BOT " << i << std::endl;
      bots.push_back(
          std::make_unique<Bot>(bot_config(i, nb_bots), consensus_config));
      auto &bot = bots[i];
      bot->_ledger->empty_database();
      bot->_ledger->init_database(block0);
      bot->_ledger->set_main_branch_tip();
    }
  };
};

}  // namespace tooling
}  // namespace neuro

#endif
