#ifndef NEURO_SRC_API_MONITORING_HPP
#define NEURO_SRC_API_MONITORING_HPP

#include <chrono>
#include <messages/Message.hpp>
#include "rest.pb.h"

namespace neuro {

class Bot;

namespace api {

class Monitoring {
  Bot* _bot;
  using TimePoint = std::chrono::time_point<std::chrono::system_clock>;
  const TimePoint _starting_time = std::chrono::system_clock::now();

 public:
  explicit Monitoring(Bot* bot);
  std::chrono::seconds::rep uptime() const;
  std::time_t last_block_ts() const;
  messages::BlockHeight current_height() const;
  uint32_t nb_blocks_since(std::time_t duration_in_s) const;
  uint32_t nb_transactions_since(std::time_t duration_in_s) const;
  uint32_t nb_blocks_1h() const;
  uint32_t nb_transactions_1h() const;
  uint32_t nb_blocks_5m() const;
  uint32_t nb_transactions_5m() const;
  float average_block_propagation_since(std::time_t since) const;
  float average_block_propagation_5m() const;
  float average_block_propagation_1h() const;
  messages::Status::Bot resource_usage() const;
  messages::Status complete_status() const;
  messages::Status::FileSystem filesystem_usage() const;
  messages::Status::PeerCount peer_count() const;
};

}  // namespace api
}  // namespace neuro

#endif  // NEURO_SRC_API_MONITORING_HPP