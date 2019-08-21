#ifndef NEURO_SRC_API_MONITORING_HPP
#define NEURO_SRC_API_MONITORING_HPP

#include <chrono>
#include "rest.pb.h"

namespace neuro {

class Bot;

namespace api {

class Monitoring {
  Bot* _bot;
  using TimePoint = std::chrono::time_point<std::chrono::system_clock>;
  const TimePoint _starting_time = std::chrono::system_clock::now();

 public:
  Monitoring(Bot* bot);
  double uptime() const;
  std::time_t last_block_ts() const;
  int current_height() const;
  int nb_blocks_since(std::time_t duration_in_s) const;
  int nb_transactions_since(std::time_t duration_in_s) const;
  int nb_blocks_1h() const;
  int nb_transactions_1h() const;
  int nb_blocks_5m() const;
  int nb_transactions_5m() const;
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
