#include "Monitoring.h"
#include <rest.pb.h>
#include <sys/resource.h>
#include <sys/time.h>

using Rusage = struct rusage;

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

messages::Rest::Bot Monitoring::resource_usage() {
  messages::Rest::Bot bot;

  Rusage usage;
  getrusage(RUSAGE_SELF, &usage);

  auto current_uptime = uptime();
  bot.set_uptime(current_uptime);
  bot.set_utime(usage.ru_utime.tv_sec);
  bot.set_stime(usage.ru_stime.tv_sec);
  double vtime = usage.ru_utime.tv_sec + usage.ru_stime.tv_sec;
  bot.set_cpu_usage(100 * vtime / current_uptime);
  bot.set_memory(usage.ru_maxrss);
  bot.set_net_in(usage.ru_msgrcv);
  bot.set_net_out(usage.ru_msgsnd);
  return bot;
}

messages::Rest::Status Monitoring::complete_status() {
  messages::Rest::Status status;
  status.mutable_blockchain()->set_last_block_ts(last_block_ts());
  status.mutable_blockchain()->set_current_height(current_height());
  status.mutable_bot()->CopyFrom(resource_usage());
  return status;
}
} // namespace neuro
