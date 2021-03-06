#ifndef NEURO_SRC_NETWORKING_NETWORKING_HPP
#define NEURO_SRC_NETWORKING_NETWORKING_HPP

#include <memory>
#include <random>
#include <vector>

#include "common/types.hpp"
#include "config.pb.h"
#include "messages.pb.h"
#include "messages/Queue.hpp"
#include "messages/Subscriber.hpp"
#include "networking/TransportLayer.hpp"
#include "networking/tcp/Connection.hpp"
#include "networking/tcp/Tcp.hpp"

namespace neuro {
namespace networking {

class Networking {
 private:
  std::unique_ptr<TransportLayer> _transport_layer;
  messages::Queue *_queue;
  crypto::Ecc *_keys;

  mutable std::uniform_int_distribution<int> _dist;

 public:
  Networking(messages::Queue *_queue, crypto::Ecc *keys, messages::Peers *peers,
             messages::config::Networking *config);

  TransportLayer::SendResult send(const messages::Message &message,
                                  Connection::ID id) const;
  TransportLayer::SendResult send_one(const messages::Message &message) const;
  TransportLayer::SendResult send_all(const messages::Message &message) const;
  bool reply(std::shared_ptr<messages::Message> message) const;
  messages::Peers *connected_peers() const;
  std::vector<messages::Peer *> peers() const;
  std::string pretty_peers() const;
  std::size_t peer_count() const;
  std::vector<std::shared_ptr<messages::Peer>> remote_peers() const;
  void join();

  bool terminate(const Connection::ID id);
  Port listening_port() const;
  bool connect(std::shared_ptr<messages::Peer> peer);
  std::shared_ptr<messages::Peer> find_peer(Connection::ID id);
  void clean_old_connections(int delta);
};

}  // namespace networking
}  // namespace neuro

#endif /* NEURO_SRC_NETWORKING_HPP */
