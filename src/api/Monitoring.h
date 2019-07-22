#ifndef CORE_MONITORING_H
#define CORE_MONITORING_H

#include "Bot.hpp"
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
};

}


#endif //CORE_MONITORING_H
