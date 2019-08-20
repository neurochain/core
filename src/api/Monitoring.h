#ifndef CORE_MONITORING_H
#define CORE_MONITORING_H

#include "Bot.hpp"
#include <rest.pb.h>
namespace neuro {

class Monitoring {
  Bot& _bot;
  using TimePoint = std::chrono::time_point<std::chrono::system_clock>;
  const TimePoint _starting_time = std::chrono::system_clock::now();

public:
  Monitoring(Bot& b);
  std::chrono::seconds::rep uptime();
  int last_block_ts();
  int current_height();
  messages::Status::Bot resource_usage();
  messages::Status complete_status();
  messages::Status::FileSystem filesystem_usage();
  messages::Status::PeerCount peer_count() const;
};

}


#endif //CORE_MONITORING_H
