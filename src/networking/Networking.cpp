#include <cassert>
#include <limits>
#include <string>

#include "common/logger.hpp"
#include "networking/Networking.hpp"

namespace neuro {
namespace networking {

Networking::Networking(std::shared_ptr<messages::Queue> queue)
    : _queue(queue), _dist(0, std::numeric_limits<uint32_t>::max()) {
  _queue->run();

  // _subscriber.subscribe(
  //     messages::Type::kConnectionClosed,
  //     [this](const messages::Header &header, const messages::Body &body) {
  //       this->remove_connection(header, body);
  //     });
}

void Networking::remove_connection(const messages::Header &header,
                                   const messages::Body &body) {
  // from header get the peer and then the transport layer id and connection id
  // auto peer = header.peer();
  // if (!peer.has_transport_layer_id() || !peer.has_connection_id()) {
  //   LOG_WARNING << this << " Traying to remove connection but not necessary
  //   info in message::Header"; return;
  // }

  // _transport_layers[peer.transport_layer_id()]->terminated(peer.connection_id());
}

bool Networking::send(std::shared_ptr<messages::Message> message,
                      ProtocolType type) {
  message->mutable_header()->set_id(_dist(_rd));
  for (auto &transport_layer : _transport_layers) {
    if (!transport_layer->send(message, type)) {
      return false;
    }
  }
  return true;
}

bool Networking::send_unicast(std::shared_ptr<messages::Message> message,
                              ProtocolType type) {
  assert(message->header().has_peer());
  return _transport_layers[message->header().peer().transport_layer_id()]
      ->send_unicast(message, type);
}

std::shared_ptr<Tcp> Networking::create_tcp(
    std::shared_ptr<messages::Queue> queue, std::shared_ptr<crypto::Ecc> keys,
    const Port port) {
  const auto id = _transport_layers.size();
  auto tcp = std::make_shared<Tcp>(port, id, _queue, keys);
  LOG_INFO << this << " Accepting connections on port " << port;
  _transport_layers.push_back(tcp);
  return tcp;
}

std::size_t Networking::peer_count() const {
  std::size_t r = 0;
  for (const auto &transport_layer : _transport_layers) {
    r += transport_layer->peer_count();
  }

  return r;
}

void Networking::join() {
  for (const auto& layer : _transport_layers) {
    layer->join();
  }
}

Networking::~Networking() { LOG_DEBUG << this << " Networking killed"; }

}  // namespace networking
}  // namespace neuro
