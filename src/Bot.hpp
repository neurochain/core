#ifndef NEURO_SRC_BOT_HPP
#define NEURO_SRC_BOT_HPP

#include <memory>
#include "crypto/Ecc.hpp"
#include "messages/Message.hpp"
#include "messages/Queue.hpp"
#include "messages/Subscriber.hpp"
#include "networking/Networking.hpp"
#include "networking/tcp/Tcp.hpp"
#include "ledger/LedgerMongodb.hpp"
#include "rest/Rest.hpp"

namespace neuro {

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

 private:
  bool init();

  void handler_hello(const messages::Header &header,
                     const messages::Body &body);
  void handler_world(const messages::Header &header,
                     const messages::Body &body);
  void handler_connection(const messages::Header &header,
                          const messages::Body &body);
  void handler_deconnection(const messages::Header &header,
                            const messages::Body &body);
  bool next_to_connect(messages::Peer **out_peer);
  bool load_keys(const messages::config::Config &config);
  void subscribe();
  
 public:
  Bot(const std::string &configuration_path);
  Bot(std::istream &bot_stream);
  Bot(const Bot &) = delete;
  Bot(Bot &&) = delete;

  virtual ~Bot();  // save_config(_config);

  void stop();
  void join();
  Bot::Status status() const;
  void keep_max_connections();
  std::shared_ptr<networking::Networking> networking();
  std::shared_ptr<messages::Queue> queue();
  void subscribe(const messages::Type type,
                 messages::Subscriber::Callback callback);
};

std::ostream &operator<<(std::ostream &os, const neuro::Bot &b);
std::ostream &operator<<(
    std::ostream &os,
    const google::protobuf::RepeatedPtrField<neuro::messages::Peer> &peers);

}  // namespace neuro

#endif /* NEURO_SRC_BOT_HPP */
