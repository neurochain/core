#ifndef NEURO_SRC_NETWORKING_NETWORKING_HPP
#define NEURO_SRC_NETWORKING_NETWORKING_HPP

#include <memory>
#include <random>
#include <vector>

#include "common/types.hpp"
#include "messages.pb.h"
#include "messages/Queue.hpp"
#include "messages/Subscriber.hpp"
#include "networking/tcp/Connection.hpp"
#include "networking/tcp/Tcp.hpp"

namespace neuro {
namespace networking {

class Networking {
  public:
   using RemoteKey = TransportLayer::RemoteKey;

  private:
   std::shared_ptr<TransportLayer> _transport_layer;
   std::random_device _rd;
   std::shared_ptr<messages::Queue> _queue;
   std::uniform_int_distribution<int> _dist;

  public:
   Networking(std::shared_ptr<messages::Queue>& _queue);
   ~Networking();

   std::shared_ptr<Tcp> create_tcp(std::shared_ptr<crypto::Ecc> keys,
                                   const Port port,
                                   const std::size_t max_connections);
   bool send(messages::Message& message);
   bool send_unicast(
       const RemoteKey& id, messages::Message& message);
   std::set<PeerPool::PeerPtr> connected_peers(const PeerPool& peer_poll) const;
   std::size_t peer_count() const;
   void keep_max_connections(const PeerPool& peer_pool);
   void join();
};

}  // namespace networking
}  // namespace neuro

#endif /* NEURO_SRC_NETWORKING_HPP */
