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
  std::vector<messages::Address> addresses;
  std::unordered_map<messages::Address, uint32_t> addresses_indexes;

  messages::config::Config bot_config(const int i) const {
    messages::config::Config config;
    auto networking = config.mutable_networking();
    networking->set_max_connections(3);
    auto tcp = networking->mutable_tcp();
    tcp->set_port(1337 + i);
    tcp->set_endpoint("localhost");
    auto new_keys = networking->add_keys();
    new_keys->mutable_key_pub()->CopyFrom(keys[i].key_pub());
    new_keys->mutable_key_priv()->CopyFrom(keys[i].key_priv());
    auto database = config.mutable_database();
    database->set_url(db_url);
    database->set_db_name(db_name + std::to_string(i));
    database->set_block0_format(messages::config::Database::PROTO);
    database->set_block0_path("data.0.testnet");
    return config;
  }

  FullSimulator(const std::string &db_url, const std::string &db_name,
                const int nb_bots, const messages::NCCAmount ncc_block0)
      : db_url(db_url), db_name(db_name), keys(nb_bots) {
    for (size_t i = 0; i < keys.size(); i++) {
      auto &address = addresses.emplace_back(keys[i].key_pub());
      addresses_indexes.insert({address, i});
    }
    for (int i = 0; i < nb_bots; i++) {
      bots.push_back(std::make_unique<Bot>(bot_config(i)));
    }
  };
};

}  // namespace tooling
}  // namespace neuro

#endif
