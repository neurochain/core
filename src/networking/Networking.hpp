#ifndef NEURO_SRC_NETWORKING_NETWORKING_HPP
#define NEURO_SRC_NETWORKING_NETWORKING_HPP

#include <memory>
#include <random>
#include <vector>

#include "messages.pb.h"
#include "messages/Queue.hpp"
#include "messages/Subscriber.hpp"
#include "networking/TransportLayer.hpp"
#include "networking/tcp/Connection.hpp"

namespace neuro {
namespace networking {

class Networking {
private:
  std::vector<std::shared_ptr<TransportLayer>> _transport_layers;

  std::random_device _rd;
  std::shared_ptr<messages::Queue> _queue;
  std::uniform_int_distribution<int> _dist;
  messages::Subscriber _subscriber;

  void remove_connection(const messages::Header &header,
			 const messages::Body &body);

public:
  Networking(std::shared_ptr<messages::Queue> _queue);
  ~Networking();

  TransportLayer::ID push(std::shared_ptr<TransportLayer> transport_layer);
  void send(std::shared_ptr<messages::Message> message);
  void send_unicast(std::shared_ptr<messages::Message> message);
  messages::Peers connected_peers() const;

  void stop();
  void join();
};

} // namespace networking
} // namespace neuro

#endif /* NEURO_SRC_NETWORKING_HPP */
