#include <cassert>
#include <limits>
#include <string>

#include "common/logger.hpp"
#include "crypto/Ecc.hpp"
#include "networking/Networking.hpp"

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
      std::make_unique<Tcp>(config->tcp().port(), queue, peers, _keys);
}

TransportLayer::SendResult Networking::send(
    std::shared_ptr<messages::Message> message) const {
  message->mutable_header()->set_id(_dist(_rd));
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
