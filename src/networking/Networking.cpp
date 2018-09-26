#include <cassert>
#include <limits>
#include <string>

#include "common/logger.hpp"
#include "networking/Networking.hpp"

namespace neuro {
namespace networking {

Networking::Networking(std::shared_ptr<messages::Queue> queue)
    : _queue(queue), _dist(0, std::numeric_limits<uint32_t>::max()),
      _subscriber(queue) {
  _queue->run();

  // messages::Type type = messages::Type::kConnectionClosed;
  // messages::Subscriber::Callback cb = [this](const messages::Header &header,
  //                                            const messages::Body &body) {
  //   this->remove_connection(header, body);
  // };
  //_subscriber.subscribe(type, cb);
  _subscriber.subscribe(
      messages::Type::kConnectionClosed,
      [this](const messages::Header &header, const messages::Body &body) {
        this->remove_connection(header, body);
      });
}

  void Networking::remove_connection(const messages::Header &header,
                       const messages::Body &body) {

  // auto remove_connection =
  //     static_cast<const messages::RemoveConnection *>(body.get());
  // auto transport_layer_id = remove_connection->transport_layer_id;
  // auto connection_id = remove_connection->connection_id;

  // for (auto it : _transport_layers) {
  //   auto tcpit = static_cast<networking::Tcp *>(it.get());
  //   if (tcpit->id() == transport_layer_id) {
  //     tcpit->terminated(connection_id);
  //   }
  // }
}

void Networking::send(std::shared_ptr<messages::Message> message) {
  message->mutable_header()->set_id(_dist(_rd));
  for (auto &transport_layer : _transport_layers) {
    transport_layer->send(message);
  }
}

void Networking::send_unicast(std::shared_ptr<messages::Message> message) {
  assert(message->header().has_peer());
  _transport_layers[message->header().peer().transport_layer_id()]
      ->send_unicast(message);
}

TransportLayer::ID
Networking::push(std::shared_ptr<TransportLayer> transport_layer) {
  _transport_layers.push_back(transport_layer);
  transport_layer->run();
  const auto id = _transport_layers.size() - 1;
  transport_layer->id(id);
  return id;
}

void Networking::stop() {
  for (const auto transport_layer : _transport_layers) {
    transport_layer->stop();
  }
}

void Networking::join() {
  for (const auto transport_layer : _transport_layers) {
    transport_layer->join();
  }
}

Networking::~Networking() { _queue->quit(); }

} // namespace networking
} // namespace neuro
