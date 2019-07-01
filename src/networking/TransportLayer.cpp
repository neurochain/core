#include "networking/TransportLayer.hpp"
#include "common/logger.hpp"
#include "messages/Queue.hpp"

namespace neuro {
namespace networking {

TransportLayer::TransportLayer(messages::Queue *queue, messages::Peers *peers,
                               crypto::Ecc *keys)
    : _queue(queue), _peers(peers), _keys(keys) {}

}  // namespace networking
}  // namespace neuro
