#include "networking/TransportLayer.hpp"
#include "common/logger.hpp"
#include "messages/Queue.hpp"

namespace neuro {
namespace networking {

TransportLayer::TransportLayer(const ID id,
                               messages::Queue *queue,
			       messages::Peers *peers,
                               std::shared_ptr<crypto::Ecc> keys)
  : _id(id), _queue(queue), _peers(peers), _keys(keys) {}

TransportLayer::ID TransportLayer::id() const { return _id; }

}  // namespace networking
}  // namespace neuro
