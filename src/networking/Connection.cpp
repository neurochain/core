#include <cassert>

#include "networking/Connection.hpp"

namespace neuro {
namespace networking {

Connection::Connection(const std::shared_ptr<messages::Queue>& queue)
    : _queue(queue) {
  assert(_queue != nullptr);
}

}  // namespace networking
}  // namespace neuro
