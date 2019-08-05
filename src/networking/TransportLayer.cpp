#include "messages/Queue.hpp"
#include "networking/TransportLayer.hpp"

namespace neuro {
namespace networking {

TransportLayer::TransportLayer(messages::Queue *queue, messages::Peers *peers,
                               crypto::Ecc *keys)
    : _queue(queue), _peers(peers), _keys(keys) {}

}  // namespace networking
}  // namespace neuro
