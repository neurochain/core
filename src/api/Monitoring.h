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
  double uptime();
  int last_block_ts();
  int current_height();
  messages::Rest::Bot resource_usage();
  messages::Rest::Status complete_status();
};

}


#endif //CORE_MONITORING_H
