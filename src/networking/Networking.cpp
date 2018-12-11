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

bool Networking::send(const std::shared_ptr<messages::Message>& message) {
  message->mutable_header()->set_id(_dist(_rd));
  return _transport_layer->send(message);
}

bool Networking::send_unicast(const RemoteKey& key, const std::shared_ptr<messages::Message>& message) {
  assert(message->header().has_peer());
  return _transport_layer->send_unicast(key, message);
}

std::shared_ptr<Tcp> Networking::create_tcp(std::shared_ptr<crypto::Ecc> keys,
    const Port port) {
  std::shared_ptr<Tcp> tcp = std::make_shared<Tcp>(port, _queue, keys);
  _transport_layer = tcp;
  return tcp;
}

std::size_t Networking::peer_count() const {
  return _transport_layer->peer_count();
}

void Networking::join() { _transport_layer->join(); }

Networking::~Networking() { LOG_DEBUG << this << " Networking killed"; }

}  // namespace networking
}  // namespace neuro
