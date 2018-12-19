#ifndef NEURO_SRC_NETWORKING_TRANSPORT_LAYER_HPP
#define NEURO_SRC_NETWORKING_TRANSPORT_LAYER_HPP

#include <thread>

#include "common/logger.hpp"
#include "common/types.hpp"
#include "networking/PeerPool.hpp"

namespace neuro {
namespace messages {
class Queue;
}  // namespace messages

namespace crypto {
class Ecc;
}  // namespace crypto

namespace networking {
class Networking;

class TransportLayer {
  public:
   using RemoteKey = Buffer;

  protected:
   std::shared_ptr<messages::Queue> _queue;
   std::shared_ptr<crypto::Ecc> _keys;

  public:
   TransportLayer(std::shared_ptr<messages::Queue> queue,
                  std::shared_ptr<crypto::Ecc> keys);

   virtual bool send(const messages::Message& message) = 0;
   virtual bool send_unicast(const RemoteKey& id, const messages::Message& message) = 0;
   virtual std::set<PeerPool::PeerPtr> connected_peers(const PeerPool& peer_pool) const = 0;
   virtual void keep_max_connections(const PeerPool& peer_pool) = 0;
   virtual std::size_t peer_count() const = 0;
   virtual ~TransportLayer(){};
   virtual void join() = 0;
};

}  // namespace networking

}  // namespace neuro

#endif /* NEURO_SRC_TRANSPORT_LAYER_HPP */
