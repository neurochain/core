#ifndef NEURO_SRC_NETWORKING_CONNECTION_HPP
#define NEURO_SRC_NETWORKING_CONNECTION_HPP

#include "common/types.hpp"
#include "networking/TransportLayer.hpp"

namespace neuro {
namespace networking {

class Connection {
 protected:
  std::shared_ptr<messages::Queue> _queue;

 public:
  Connection(const std::shared_ptr<messages::Queue>& queue);
  virtual ~Connection() {}
};

}  // namespace networking
}  // namespace neuro

#endif /* NEURO_NETWORKING_CONNECTION_HPP */
