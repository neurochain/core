#ifndef NEURO_SRC_API_MONITORING_HPP
#define NEURO_SRC_API_MONITORING_HPP

#include "rest.pb.h"
#include <chrono>

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
  messages::Status::Bot resource_usage() const;
  messages::Status complete_status() const;
  messages::Status::FileSystem filesystem_usage() const;
  messages::Status::PeerCount peer_count() const;
};

} // namespace api
} // naemspace neuro


#endif //NEURO_SRC_API_MONITORING_HPP
