#include "networking/Connection.hpp"

namespace neuro {
namespace networking {

Connection::ID Connection::id() const { return _id; }

std::shared_ptr<messages::Queue> Connection::queue() const { return _queue; }

TransportLayer::ID Connection::transport_layer_id() const {
  return _transport_layer_id;
}

}  // namespace networking
}  // namespace neuro
