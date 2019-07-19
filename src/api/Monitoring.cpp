#include "Monitoring.h"

namespace neuro {

Monitoring::Monitoring(Bot &b) : _bot(b) {
  _bot.subscribe(messages::Type::kEvent, [this](const messages::Header &header,
                                                const messages::Body &body) {
    switch (body.event().type()) {
    case messages::Event_Type_BLOCKMINED:
      _last_block_timestamp = std::chrono::system_clock::now();
      break;
    }
  });
}

double Monitoring::uptime() {
  auto now = std::chrono::system_clock::now();
  std::chrono::duration<double> diff = now - _starting_time;
  return diff.count();
}

Monitoring::TimePoint::duration Monitoring::time_since_last_block() {
  return _last_block_timestamp.time_since_epoch().count();
}

} // namespace neuro
