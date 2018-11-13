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
#include "networking/Networking.hpp"
#include "networking/tcp/Tcp.hpp"
#include "rest/Rest.hpp"
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
  messages::config::Config _config;
  std::shared_ptr<crypto::Ecc> _keys;
  messages::Subscriber _subscriber;
  std::shared_ptr<ledger::Ledger> _ledger;
  std::shared_ptr<rest::Rest> _rest;
  std::shared_ptr<consensus::Consensus> _consensus;
  std::shared_ptr<boost::asio::io_context> _io_context;
  boost::asio::steady_timer _update_timer;
  std::unordered_set<int32_t> _request_ids;
  std::thread _io_context_thread;

  // for the peers
  messages::config::Tcp *_tcp_config;
  std::size_t _connected_peers{0};
  std::size_t _max_connections;

  std::shared_ptr<networking::Tcp> _tcp;
  messages::Peer::Status _keep_status;
  messages::config::Config::SelectionMethod _selection_method;

  mutable std::mutex _mutex_connections;
  mutable std::mutex _mutex_quitting;
  bool _quitting{false};

  uint32_t _update_time{1};  // In seconds

 private:
  bool init();

  void handler_hello(const messages::Header &header,
                     const messages::Body &body);
  void handler_world(const messages::Header &header,
                     const messages::Body &body);
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
  bool next_to_connect(messages::Peer **out_peer);
  bool load_keys(const messages::config::Config &config);
  bool load_networking(messages::config::Config *config);
  void subscribe();
  void regular_update();
  bool update_ledger();
  void update_peerlist();

  std::optional<messages::Peer *> add_peer(const messages::Peer &peer) {
    std::optional<messages::Peer *> res;
    messages::KeyPub my_key_pub;
    auto peers = _tcp_config->mutable_peers();
    _keys->public_key().save(&my_key_pub);

    if (peer.key_pub() == my_key_pub) {
      return res;
    }

    // auto got = std::find(peers->begin(), peers->end(), peer);
    for (auto &p : *peers) {
      if (p.key_pub() == peer.key_pub()) {
        res = &p;
        return res;
      }
    }

    // if (got != peers->end()) {
    //   res = &(*got);
    //   return res;
    // }

    res = _tcp_config->add_peers();
    (*res)->CopyFrom(peer);

    return res;
  }

  template <typename T>
  void add_peers(const T &remote_peers) {
    for (const auto &peer : remote_peers) {
      add_peer(peer);
    }
  }

 public:
  Bot(const std::string &configuration_path);
  Bot(std::istream &bot_stream);
  Bot(const Bot &) = delete;
  Bot(Bot &&) = delete;

  virtual ~Bot();  // save_config(_config);

  const std::vector<messages::Peer> connected_peers() const;
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
