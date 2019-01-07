#include <cassert>

#include "networking/Connection.hpp"

namespace neuro {
namespace networking {

Connection::Connection(const ID id, messages::Queue * queue)
    : _id(id), _queue(queue) {
  assert(_queue != nullptr);
}

Connection::ID Connection::id() const { return _id; }

}  // namespace networking
}  // namespace neuro
