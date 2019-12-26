#include "networking/Networking.hpp"
#include "common/types.hpp"
#include "config.pb.h"
#include "crypto/Ecc.hpp"
#include "messages/Peer.hpp"
#include "messages/Peers.hpp"
#include "messages/Queue.hpp"
#include "networking/Connection.hpp"
#include "networking/TransportLayer.hpp"

namespace neuro {
namespace networking {

Networking::Networking(messages::Queue *queue, crypto::Ecc *keys,
                       messages::Peers *peers,
                       messages::config::Networking *config)
    : _queue(queue),
      _keys(keys),
      _dist(0, std::numeric_limits<uint32_t>::max()) {
  _queue->run();
  _transport_layer = std::make_unique<Tcp>(queue, peers, _keys, *config);
}

bool Networking::reply(std::shared_ptr<messages::Message> message) const {
  return _transport_layer->reply(message);
}

TransportLayer::SendResult Networking::send(const messages::Message &message,
                                            const Connection::ID id) const {
  return _transport_layer->send(message, id);
}

TransportLayer::SendResult Networking::send_one(
    const messages::Message &message) const {
  return _transport_layer->send_one(message);
}

TransportLayer::SendResult Networking::send_all(
    const messages::Message &message) const {
  return _transport_layer->send_all(message);
}

/**
 * count the number of active connection (either accepted one or attempting one)
 * \return the number of active connection
 */
std::size_t Networking::peer_count() const {
  return _transport_layer->peer_count();
}

std::vector<std::shared_ptr<messages::Peer>> Networking::remote_peers() const {
  return _transport_layer->remote_peers();
}

std::string Networking::pretty_peers() const {
  std::stringstream result;
  auto remote_peers = this->remote_peers();
  for (const auto &peer : remote_peers) {
    result << " " << peer->endpoint() << ":" << peer->port() << ":"
           << _Peer_Status_Name(peer->status()) << ":" << peer->connection_id();
  }
  return result.str();
}

void Networking::join() { _transport_layer->join(); }

bool Networking::terminate(const Connection::ID id) {
  return _transport_layer->terminate(id);
}

Port Networking::listening_port() const {
  return _transport_layer->listening_port();
}

bool Networking::connect(std::shared_ptr<messages::Peer> peer) {
  return _transport_layer->connect(peer);
}

void Networking::clean_old_connections(int delta) {
  _transport_layer->clean_old_connections(delta);
}

/**
 * Find a peer associated with a connection
 * \param id an identifiant of a connection
 * \return the associated peer for the connection
 */
std::shared_ptr<messages::Peer> Networking::find_peer(Connection::ID id) {
  return _transport_layer->find_peer(id);
}

}  // namespace networking
}  // namespace neuro
