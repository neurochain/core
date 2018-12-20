#ifndef NEURO_SRC_BOT_HPP
#define NEURO_SRC_BOT_HPP

#include <memory>
#include "Bot.hpp"
#include "consensus/Consensus.hpp"
#include "crypto/Ecc.hpp"
#include "ledger/LedgerMongodb.hpp"
#include "messages/Message.hpp"
#include "messages/Queue.hpp"
#include "messages/Subscriber.hpp"
#include "messages/config/Config.hpp"
#include "networking/Networking.hpp"
#include "networking/tcp/Tcp.hpp"
#include "rest/Rest.hpp"
#include "networking/PeerPool.hpp"

namespace neuro {

namespace tests {
class BotTest;
}

class Bot {
 public:
  struct Status {
    std::size_t connected_peers;
    std::size_t max_connections;
    std::size_t peers_in_pool;

    Status(const std::size_t connected, const std::size_t max,
           const std::size_t peers)
        : connected_peers(connected),
          max_connections(max),
          peers_in_pool(peers) {}
  };

 private:
  std::shared_ptr<messages::Queue> _queue;
  std::shared_ptr<networking::Networking> _networking;
  std::size_t _max_connections;
  std::shared_ptr<networking::PeerPool> _peer_pool;
  messages::Subscriber _subscriber;
  std::shared_ptr<boost::asio::io_context> _io_context;
  messages::config::Config _config;
  boost::asio::steady_timer _update_timer;
  std::shared_ptr<crypto::Ecc> _keys;
  std::shared_ptr<ledger::Ledger> _ledger;
  std::shared_ptr<rest::Rest> _rest;
  std::shared_ptr<consensus::Consensus> _consensus;
  std::unordered_set<int32_t> _request_ids;
  std::thread _io_context_thread;

  messages::config::Config::SelectionMethod _selection_method;

  mutable std::mutex _mutex_connections;
  mutable std::mutex _mutex_quitting;
  bool _quitting{false};

  uint32_t _update_time{1};  // In seconds

 private:
  bool init();

  void update_connection_graph();
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
  bool load_keys(const messages::config::Config &config);
  bool load_networking(messages::config::Config *config);
  void subscribe();
  void regular_update();
  bool update_ledger();
  void update_peerlist();

 public:
  std::string peer_list_str() const;
  Bot(const messages::config::Config &config);
  Bot(const std::string &config_path);
  Bot(const Bot &) = delete;
  Bot(Bot &&) = delete;

  void join();

  virtual ~Bot();  // save_config(_config);

  std::set<networking::PeerPool::PeerPtr> connected_peers() const;
  std::size_t nb_connected_peers() const;
  Bot::Status status() const;
  void keep_max_connections();
  std::shared_ptr<networking::Networking> networking();
  std::shared_ptr<messages::Queue> queue();
  void subscribe(const messages::Type type,
                 messages::Subscriber::Callback callback);

  void publish_transaction(const messages::Transaction &transaction) const;

  friend class neuro::tests::BotTest;
};

std::ostream &operator<<(std::ostream &os, const neuro::Bot &b);
std::ostream &operator<<(
    std::ostream &os,
    const google::protobuf::RepeatedPtrField<neuro::messages::Peer> &peers);

}  // namespace neuro

#endif /* NEURO_SRC_BOT_HPP */
