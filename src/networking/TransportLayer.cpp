#include "networking/TransportLayer.hpp"
#include "common/logger.hpp"
#include "messages/Queue.hpp"

namespace neuro {
namespace networking {

TransportLayer::TransportLayer(std::shared_ptr<messages::Queue> queue,
                               std::shared_ptr<crypto::Ecc> keys)
    : _queue(queue), _keys(keys), _id(-1) {}

TransportLayer::ID TransportLayer::id() const { return _id; }
void TransportLayer::id(const TransportLayer::ID id) { _id = id; }

void TransportLayer::run() {
  _thread = std::thread([this]() { this->_run(); });
}

void TransportLayer::stop() { this->_stop(); }

void TransportLayer::join() { _thread.join(); }
TransportLayer::~TransportLayer() {
  if (_thread.joinable()) {
    _thread.join();
  }
}

} // namespace networking
} // namespace neuro
