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
 private:
  std::unique_ptr<TransportLayer> _transport_layer;

  std::random_device _rd;
  messages::Queue* _queue;
  std::uniform_int_distribution<int> _dist;

 public:
  Networking(messages::Queue * _queue);
  ~Networking();

  std::shared_ptr<Tcp> create_tcp(messages::Queue * queue,
                                  std::shared_ptr<crypto::Ecc> keys,
                                  const Port port);
  TransportLayer::SendResult send(std::shared_ptr<messages::Message> message);
  bool send_unicast(std::shared_ptr<messages::Message> message);
  messages::Peers connected_peers() const;
  std::size_t peer_count() const;
  void join();
};

}  // namespace networking
}  // namespace neuro

#endif /* NEURO_SRC_NETWORKING_HPP */
