#include "Monitoring.h"
#include <rest.pb.h>
#include <sys/resource.h>
#include <sys/time.h>
#include <sys/statfs.h>

using Rusage = struct rusage;
using Statfs = struct statfs;

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

messages::Status::Bot Monitoring::resource_usage() {
  messages::Status::Bot bot;

  Rusage usage;
  getrusage(RUSAGE_SELF, &usage);

  auto current_uptime = uptime();
  bot.set_uptime(static_cast<int>(current_uptime));
  bot.set_utime(usage.ru_utime.tv_sec);
  bot.set_stime(usage.ru_stime.tv_sec);
  double vtime = usage.ru_utime.tv_sec + usage.ru_stime.tv_sec;
  bot.set_cpu_usage(100 * vtime / current_uptime);
  bot.set_memory(usage.ru_maxrss);
  bot.set_net_in(usage.ru_msgrcv);
  bot.set_net_out(usage.ru_msgsnd);

  return bot;
}

messages::Status::FileSystem Monitoring::filesystem_usage() {
  messages::Status::FileSystem fs;
  Statfs stat;
  statfs("/", &stat);
  auto BLOCKSIZE = stat.f_bsize;
  fs.set_total_space(stat.f_blocks * BLOCKSIZE);
  fs.set_used_space((stat.f_blocks - stat.f_bfree) * BLOCKSIZE);
  fs.set_total_inode(stat.f_files);
  fs.set_used_inode(stat.f_files - stat.f_ffree);
  return fs;
}

messages::Status::PeerCount Monitoring::peer_count() const {
  messages::Status::PeerCount peerCount;
  auto connected = 0;
  auto connecting = 0;
  auto disconnected = 0;
  auto unreachable = 0;

  for (const auto peer : _bot.peers()) {
    switch (peer->status()) {
    case messages::Peer::CONNECTED:
      connected++;
      break;
    case messages::Peer::CONNECTING:
      connecting++;
      break;
    case messages::Peer::DISCONNECTED:
      disconnected++;
      break;
    case messages::Peer::UNREACHABLE:
      unreachable++;
      break;
    default:
      break;
    }
  }
  peerCount.set_connected(connected);
  peerCount.set_connecting(connecting);
  peerCount.set_disconnected(disconnected);
  peerCount.set_unreachable(unreachable);
  return peerCount;
}

messages::Status Monitoring::complete_status() {
  messages::Status status;
  status.mutable_blockchain()->set_last_block_ts(last_block_ts());
  status.mutable_blockchain()->set_current_height(current_height());
  status.mutable_bot()->CopyFrom(resource_usage());
  status.mutable_fs()->CopyFrom(filesystem_usage());
  status.mutable_peer()->CopyFrom(peer_count());
  return status;
}
} // namespace neuro
