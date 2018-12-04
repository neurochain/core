#include "networking/TransportLayer.hpp"
#include "common/logger.hpp"
#include "messages/Queue.hpp"

namespace neuro {
namespace networking {

TransportLayer::TransportLayer(const ID id,
                               std::shared_ptr<messages::Queue> queue,
                               std::shared_ptr<crypto::Ecc> keys)
    : _queue(queue), _keys(keys), _id(id) {}

TransportLayer::ID TransportLayer::id() const { return _id; }

}  // namespace networking
}  // namespace neuro
