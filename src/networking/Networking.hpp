#ifndef NEURO_SRC_NETWORKING_NETWORKING_HPP
#define NEURO_SRC_NETWORKING_NETWORKING_HPP

#include <memory>
#include <random>
#include <vector>

#include "common/types.hpp"
#include "messages.pb.h"
#include "messages/Queue.hpp"
#include "messages/Subscriber.hpp"
#include "networking/tcp/Tcp.hpp"
#include "networking/tcp/Connection.hpp"

namespace neuro {
namespace networking {

class Networking {
 private:
  std::vector<std::shared_ptr<TransportLayer>> _transport_layers;

  std::random_device _rd;
  std::shared_ptr<messages::Queue> _queue;
  std::uniform_int_distribution<int> _dist;

 public:
  Networking(std::shared_ptr<messages::Queue> _queue);
  ~Networking();

  std::pair<std::shared_ptr<Tcp>, TransportLayer::ID> create_tcp(
      std::shared_ptr<messages::Queue> queue,
      std::shared_ptr<crypto::Ecc> keys,
      ::google::protobuf::int32 port);
  void send(std::shared_ptr<messages::Message> message, ProtocolType type);
  void send_unicast(std::shared_ptr<messages::Message> message,
                    ProtocolType type);
  messages::Peers connected_peers() const;
  std::size_t peer_count() const;
  void remove_connection(const messages::Header &header,
                         const messages::Body &body);

  void stop();
  void join();
};

}  // namespace networking
}  // namespace neuro

#endif /* NEURO_SRC_NETWORKING_HPP */
