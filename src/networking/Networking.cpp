#include <cassert>
#include <limits>
#include <string>

#include "common/logger.hpp"
#include "networking/Networking.hpp"

namespace neuro {
namespace networking {

Networking::Networking(messages::Queue* queue)
    : _queue(queue), _dist(0, std::numeric_limits<uint32_t>::max()) {
  _queue->run();
}

TransportLayer::SendResult Networking::send(std::shared_ptr<messages::Message> message) {
  message->mutable_header()->set_id(_dist(_rd));
  return _transport_layer->send(message);
}

bool Networking::send_unicast(std::shared_ptr<messages::Message> message) {
  return _transport_layer->send_unicast(message);
}

std::size_t Networking::peer_count() const {
  return _transport_layer->peer_count();
}

void Networking::join() {
  _transport_layer->join();
}

Networking::~Networking() { LOG_DEBUG << this << " Networking killed"; }

}  // namespace networking
}  // namespace neuro
