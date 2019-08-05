#include "common/types.hpp"
#include "config.pb.h"
#include "crypto/Ecc.hpp"
#include "messages/Peer.hpp"
#include "messages/Peers.hpp"
#include "messages/Queue.hpp"
#include "networking/Connection.hpp"
#include "networking/Networking.hpp"
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
  _transport_layer =
      std::make_unique<Tcp>(queue, peers, _keys, *config);
}

TransportLayer::SendResult Networking::send(
    std::shared_ptr<messages::Message> message) const {
  return _transport_layer->send(message);
}

bool Networking::send_unicast(
    std::shared_ptr<messages::Message> message) const {
  return _transport_layer->send_unicast(message);
}

/**
 * count the number of active connexion (either accepted one or attempting one)
 * \return the number of active connexion
 */
std::size_t Networking::peer_count() const {
  return _transport_layer->peer_count();
}

void Networking::join() { _transport_layer->join(); }

bool Networking::terminate(const Connection::ID id) {
  return _transport_layer->terminate(id);
}

Port Networking::listening_port() const {
  return _transport_layer->listening_port();
}

bool Networking::connect(messages::Peer *peer) {
  return _transport_layer->connect(peer);
}

/**
 * Find a peer associated with a connection
 * \param id an identifiant of a connection
 * \return the associated peer for the connection
 */
std::optional<messages::Peer*> Networking::find_peer(Connection::ID id) {
  return _transport_layer->find_peer(id);
}

}  // namespace networking
}  // namespace neuro
