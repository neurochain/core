#include "networking/Connection.hpp"

namespace neuro {
namespace networking {

Connection::ID Connection::id() const {
  return _id;
}

TransportLayer::ID Connection::transport_layer_id() const {
  return _transport_layer_id;
}

}  // networking
}  // neuro
