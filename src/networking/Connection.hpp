#ifndef NEURO_SRC_NETWORKING_CONNECTION_HPP
#define NEURO_SRC_NETWORKING_CONNECTION_HPP

#include "common/types.hpp"
#include "networking/TransportLayer.hpp"

namespace neuro {
namespace networking {

class Connection {
 public:
  using ID = uint16_t;

 protected:
  ID _id;
  TransportLayer::ID _transport_layer_id;
  std::shared_ptr<messages::Queue> _queue;

 public:
  Connection(const ID id, TransportLayer::ID transport_layer_id,
             std::shared_ptr<messages::Queue> queue);
  ID id() const;
  TransportLayer::ID transport_layer_id() const;
};

}  // namespace networking
}  // namespace neuro

#endif /* NEURO_NETWORKING_CONNECTION_HPP */
