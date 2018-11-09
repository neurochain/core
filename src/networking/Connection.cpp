#include "networking/Connection.hpp"
#include "common/check.hpp"

namespace neuro {
namespace networking {

Connection::Connection(const ID id, TransportLayer::ID transport_layer_id,
                     std::shared_ptr<messages::Queue> queue)
    : _id(id), _transport_layer_id(transport_layer_id), _queue(queue) {
  CHECK(_queue != nullptr, "_queue is nullptr");
}

Connection::ID Connection::id() const { return _id; }

TransportLayer::ID Connection::transport_layer_id() const {
  return _transport_layer_id;
}

}  // namespace networking
}  // namespace neuro
