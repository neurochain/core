#include <cassert>

#include "networking/Connection.hpp"

namespace neuro {
namespace networking {

Connection::Connection(const ID id, const TransportLayer::ID transport_layer_id,
                       const std::shared_ptr<messages::Queue>& queue)
    : _id(id), _transport_layer_id(transport_layer_id), _queue(queue) {
  assert(_queue != nullptr);
}

Connection::ID Connection::id() const { return _id; }

TransportLayer::ID Connection::transport_layer_id() const {
  return _transport_layer_id;
}

}  // namespace networking
}  // namespace neuro
