#ifndef CORE_MONITORING_H
#define CORE_MONITORING_H

#include "Bot.hpp"
namespace neuro {

class Monitoring {
  using TimePoint = std::chrono::time_point<std::chrono::system_clock>;
  Bot& _bot;
  const TimePoint _starting_time = std::chrono::system_clock::now();
  TimePoint _last_block_timestamp;

public:
  Monitoring(Bot& b);
  double uptime();
  double time_since_last_block();
};

}


#endif //CORE_MONITORING_H
