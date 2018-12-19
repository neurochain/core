#include <cassert>
#include <limits>
#include <string>

#include "common/logger.hpp"
#include "networking/Networking.hpp"

namespace neuro {
namespace networking {

Networking::Networking(std::shared_ptr<messages::Queue>& queue)
    : _queue(queue), _dist(0, std::numeric_limits<uint32_t>::max()) {
  _queue->run();
}

bool Networking::send(messages::Message& message) {
  message.mutable_header()->set_id(_dist(_rd));
  return _transport_layer->send(message);
}

bool Networking::send_unicast(const RemoteKey& key, messages::Message& message) {
  return _transport_layer->send_unicast(key, message);
}

std::shared_ptr<Tcp> Networking::create_tcp(std::shared_ptr<crypto::Ecc> keys,
                                            const Port port,
                                            const std::size_t max_connections) {
  std::shared_ptr<Tcp> tcp =
      std::make_shared<Tcp>(port, _queue, keys, max_connections);
  _transport_layer = tcp;
  return tcp;
}

void Networking::keep_max_connections(const PeerPool& peer_pool) {
  _transport_layer->keep_max_connections(peer_pool);
}

std::set<PeerPool::PeerPtr> Networking::connected_peers(
    const PeerPool& peer_pool) const {
  return _transport_layer->connected_peers(peer_pool);
}

std::size_t Networking::peer_count() const {
  return _transport_layer->peer_count();
}

void Networking::join() { _transport_layer->join(); }

Networking::~Networking() { LOG_DEBUG << this << " Networking killed"; }

}  // namespace networking
}  // namespace neuro
