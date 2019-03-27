#ifndef NEURO_SRC_NETWORKING_CONNECTION_HPP
#define NEURO_SRC_NETWORKING_CONNECTION_HPP

#include "common/types.hpp"
#include "messages/Queue.hpp"

namespace neuro {
namespace networking {

class Connection {
 public:
  using ID = uint16_t;

 protected:
  ID _id;
  messages::Queue* _queue;

 public:
  Connection(const ID id, messages::Queue* queue);
  ID id() const;
};

}  // namespace networking
}  // namespace neuro

#endif /* NEURO_NETWORKING_CONNECTION_HPP */
