#include "networking/TransportLayer.hpp"
#include "common/logger.hpp"
#include "messages/Queue.hpp"

namespace neuro {
namespace networking {

TransportLayer::TransportLayer(std::shared_ptr<messages::Queue> queue,
                               std::shared_ptr<crypto::Ecc> keys)
    : _queue(queue), _keys(keys) {}

}  // namespace networking
}  // namespace neuro
