#include "Monitoring.hpp"
#include <rest.pb.h>
#include <sys/resource.h>
#include <sys/statfs.h>
#include <ctime>
#include "Bot.hpp"
#include "version.h"

namespace neuro::api {

using Rusage = struct rusage;
using Statfs = struct statfs;

Monitoring::Monitoring(Bot *bot) : _bot(bot) {}

std::chrono::seconds::rep Monitoring::uptime() const {
  using namespace std::chrono;
  auto now = system_clock::now();
  auto diff = duration_cast<seconds>(now - _starting_time);
  return diff.count();
}

std::time_t Monitoring::last_block_ts() const {
  auto last_blocks = _bot->ledger()->get_last_blocks(1);
  if (last_blocks.empty()) {
    return 0;
  }

  const auto last_block = last_blocks[0];
  return last_block.header().timestamp().data();
}

messages::BlockHeight Monitoring::current_height() const {
  return _bot->ledger()->height();
}

uint32_t Monitoring::nb_blocks_since(const std::time_t since) const {
  uint32_t total = 0;
  auto tagged_block = _bot->ledger()->get_main_branch_tip();
  auto current_time = std::time(nullptr);
  while (current_time - tagged_block.block().header().timestamp().data() <
         since) {
    total++;
    if (!_bot->ledger()->get_block(
            tagged_block.block().header().previous_block_hash(), &tagged_block,
            false)) {
      break;
    }
  }
  return total;
}

uint32_t Monitoring::nb_transactions_since(std::time_t since) const {
  int total = 0;
  messages::TaggedBlock tagged_block;
  _bot->ledger()->get_last_block(&tagged_block);
  auto current_time = std::time(nullptr);
  while (current_time - tagged_block.block().header().timestamp().data() <
         since) {
    // Add 1 for the coinbase
    total += tagged_block.block().transactions_size() + 1;
    if (!_bot->ledger()->get_block(
            tagged_block.block().header().previous_block_hash(),
            &tagged_block)) {
      break;
    }
  }
  return total;
}

float Monitoring::average_block_propagation_since(std::time_t since) const {
  int nb_blocks = 0;
  int total_propagation = 0;
  auto tagged_block = _bot->ledger()->get_main_branch_tip();
  while (std::time(nullptr) - tagged_block.block().header().timestamp().data() <
         since) {
    nb_blocks++;
    if (tagged_block.has_reception_time()) {
      int propagation = tagged_block.reception_time().data() -
                        tagged_block.block().header().timestamp().data();
      if (propagation > 0) {
        total_propagation += propagation;
      }
    }
    if (!_bot->ledger()->get_block(
            tagged_block.block().header().previous_block_hash(), &tagged_block,
            false)) {
      break;
    }
  }
  if (nb_blocks == 0) {
    return -1;
  }
  return static_cast<float>(total_propagation) / nb_blocks;
}

uint32_t Monitoring::nb_blocks_5m() const { return nb_blocks_since(300); }

uint32_t Monitoring::nb_blocks_1h() const { return nb_blocks_since(3600); }

uint32_t Monitoring::nb_transactions_5m() const {
  return nb_transactions_since(300);
}

uint32_t Monitoring::nb_transactions_1h() const {
  return nb_transactions_since(3600);
}

float Monitoring::average_block_propagation_5m() const {
  return average_block_propagation_since(300);
}

float Monitoring::average_block_propagation_1h() const {
  return average_block_propagation_since(3600);
}

messages::Status::Bot Monitoring::resource_usage() const {
  messages::Status::Bot bot;

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
  bot.set_version(GIT_COMMIT_HASH);

  return bot;
}

messages::Status::FileSystem Monitoring::filesystem_usage() const {
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

  for (const auto peer : _bot->peers()) {
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

messages::Status_BlockChain Monitoring::blockchain_health() const {
  messages::Status_BlockChain health;
  health.set_last_block_ts(last_block_ts());
  health.set_current_height(current_height());
  health.set_nb_blocks_5m(nb_blocks_5m());
  health.set_nb_blocks_1h(nb_blocks_1h());
  health.set_nb_transactions_5m(nb_transactions_5m());
  health.set_nb_transactions_1h(nb_transactions_1h());
  health.set_average_block_propagation_5m(
      average_block_propagation_5m());
  health.set_average_block_propagation_1h(
      average_block_propagation_1h());
  return health;
}

messages::Status Monitoring::fast_status() const {
  messages::Status status;
  status.mutable_bot()->CopyFrom(resource_usage());
  status.mutable_fs()->CopyFrom(filesystem_usage());
  status.mutable_peer()->CopyFrom(peer_count());
  status.mutable_blockchain()->CopyFrom(blockchain_health());
  return status;
}

messages::Status Monitoring::complete_status() const {
  messages::Status status = fast_status();
  status.mutable_blockchain()->set_mined_block(_bot->ledger()->total_nb_blocks());
  status.mutable_blockchain()->set_transaction_count(_bot->ledger()->total_nb_transactions());
  return status;
}

}  // namespace neuro::api
