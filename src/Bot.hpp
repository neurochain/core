#ifndef NEURO_SRC_BOT_HPP
#define NEURO_SRC_BOT_HPP

#include <boost/asio/steady_timer.hpp>
#include <cstdint>
#include <memory>
#include <mutex>
#include <thread>

#include "Bot.hpp"
#include "api/Api.hpp"
#include "config.pb.h"
#include "consensus/Config.hpp"
#include "consensus/Consensus.hpp"
#include "crypto/Ecc.hpp"
#include "ledger/Ledger.hpp"
#include "ledger/LedgerMongodb.hpp"
#include "messages.pb.h"
#include "messages/Message.hpp"
#include "messages/Peer.hpp"
#include "messages/Peers.hpp"
#include "messages/Queue.hpp"
#include "messages/Subscriber.hpp"
#include "messages/config/Config.hpp"
#include "networking/Networking.hpp"
#include "networking/tcp/Tcp.hpp"

namespace neuro {

namespace tests {
class BotTest;
}

namespace tooling {
class FullSimulator;

namespace tests {
class FullSimulator;
}
}  // namespace tooling

class Bot {
 public:
 private:
  messages::config::Config _config;
  std::shared_ptr<boost::asio::io_context> _io_context;
  messages::Queue _queue;
  messages::Subscriber _subscriber;
  crypto::Eccs _keys;
  messages::Peer _me;
  messages::Peers _peers;
  networking::Networking _networking;
  std::shared_ptr<ledger::Ledger> _ledger;
  boost::asio::steady_timer _update_timer;
  std::optional<consensus::Config> _consensus_config;
  std::shared_ptr<consensus::Consensus> _consensus;
  std::unique_ptr<api::Api> _api;
  std::unordered_set<int32_t> _request_ids;
  std::thread _io_context_thread;

  // for the peers
  messages::config::Tcp *_tcp_config;
  std::size_t _max_connections;
  std::size_t
      _max_incoming_connections;  //!< number of connexion this bot can accept

  mutable std::mutex _mutex_connections;
  mutable std::mutex _mutex_quitting;
  bool _quitting{false};

  uint32_t _update_time{1};  // In seconds

 private:
  bool init();

  void handler_heart_beat(const messages::Header &header,
                          const messages::Body &body);
  void handler_ping(const messages::Header &header, const messages::Body &body);
  void handler_hello(const messages::Header &header,
                     const messages::Body &body);
  void handler_world(const messages::Header &header,
                     const messages::Body &body);
  // void update_connection_graph();
  void handler_connection(const messages::Header &header,
                          const messages::Body &body);
  void handler_deconnection(const messages::Header &header,
                            const messages::Body &body);
  void handler_transaction(const messages::Header &header,
                           const messages::Body &body);
  void handler_block(const messages::Header &header,
                     const messages::Body &body);
  void handler_get_block(const messages::Header &header,
                         const messages::Body &body);
  void handler_get_peers(const messages::Header &header,
                         const messages::Body &body);
  void handler_peers(const messages::Header &header,
                     const messages::Body &body);
  bool next_to_connect(messages::Peer **out_peer);
  bool configure_networking(messages::config::Config *config);
  void subscribe();
  void regular_update();
  void send_random_transaction();
  void send_pings();
  void update_ledger();
  bool update_ledger(const std::optional<messages::Hash> &missing_block);
  void update_peerlist();

 public:
  explicit Bot(const messages::config::Config &config,
               const consensus::Config &consensus_config = consensus::Config());
  explicit Bot(const std::string &config_path);
  Bot(const Bot &) = delete;

  void join();

  virtual ~Bot();  // save_config(_config);

  const messages::Peer me() const;
  const messages::Peers &peers() const;
  const messages::_Peers remote_peers() const;
  void keep_max_connections();
  std::vector<messages::Peer *> connected_peers();
  void subscribe(const messages::Type type,
                 messages::Subscriber::Callback callback);

  bool publish_transaction(const messages::Transaction &transaction) const;
  void publish_block(const messages::Block &block) const;
  ledger::Ledger *ledger();

  friend class neuro::tests::BotTest;
  friend class neuro::tooling::FullSimulator;
  friend class neuro::tooling::tests::FullSimulator;
};

std::ostream &operator<<(std::ostream &os, const neuro::Bot &b);
std::ostream &operator<<(
    std::ostream &os,
    const google::protobuf::RepeatedPtrField<neuro::messages::Peer> &peers);

}  // namespace neuro

#endif /* NEURO_SRC_BOT_HPP */
