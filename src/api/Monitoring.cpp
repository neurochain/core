#include "Monitoring.h"

namespace neuro {

Monitoring::Monitoring(Bot &b) : _bot(b) {}

double Monitoring::uptime() {
  auto now = std::chrono::system_clock::now();
  std::chrono::duration<double> diff = now - _starting_time;
  return diff.count();
}

int Monitoring::last_block_ts() {
  auto last_blocks = _bot.ledger()->get_last_blocks(1);
  if (!last_blocks.empty()) {
    auto last_block = last_blocks[0];
    return last_block.header().timestamp().data();
  } else {
    return 0;
  }
}

int Monitoring::current_height() {
  auto last_blocks = _bot.ledger()->get_last_blocks(1);
  if (!last_blocks.empty()) {
    auto last_block = last_blocks[0];
    return last_block.header().height();
  } else {
    return 0;
  }
}

} // namespace neuro
