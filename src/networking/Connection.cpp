#include <cassert>

#include "messages/Queue.hpp"
#include "networking/Connection.hpp"

namespace neuro {
namespace networking {

Connection::Connection(const ID id, messages::Queue* queue)
    : _id(id), _queue(queue), _init_ts(std::time(nullptr)) {
  assert(_queue != nullptr);
}

Connection::ID Connection::id() const { return _id; }
std::time_t Connection::init_ts() const { return _init_ts; }

}  // namespace networking
}  // namespace neuro
