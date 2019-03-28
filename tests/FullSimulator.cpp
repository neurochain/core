#include <gtest/gtest.h>
#include <Bot.hpp>
#include "common/types.hpp"
#include "messages/Message.hpp"
#include "messages/config/Config.hpp"

namespace neuro {
namespace tests {

std::vector<Bot> simulate(int nb_bots) {
  std::vector<Bot> bots;
  for (int i = 0; i < nb_bots; i++) {
    messages::config::Config config;
    auto networking = config.mutable_networking();
    networking->set_max_connections(3);
    auto tcp = networking->mutable_tcp();
    tcp->set_port(1337 + i);
    tcp->set_endpoint("localhost");

    // auto database = config.mutable_database();
  }
  return bots;
}

TEST(FULL_SIMULATOR, full_simulator) {}

}  // namespace tests
}  // namespace neuro
