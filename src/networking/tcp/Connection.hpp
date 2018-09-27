#ifndef NEURO_SRC_NETWORKING_TCP_CONNECTION_HPP
#define NEURO_SRC_NETWORKING_TCP_CONNECTION_HPP

#include <boost/asio.hpp>
#include <cmath>
#include <iostream>
#include <stdio.h>

#include "messages.pb.h"
#include "networking/Connection.hpp"
#include "networking/TransportLayer.hpp"
#include "networking/tcp/HeaderPattern.hpp"

namespace neuro {
namespace messages {
class Queue;
} // namespace messages
namespace networking {
namespace tcp {
using boost::asio::ip::tcp;

class Connection : public networking::Connection {
 private:
  Buffer _header{sizeof(HeaderPattern), 0};
  Buffer _buffer{128, 0};
  std::shared_ptr<tcp::socket> _socket;
  std::shared_ptr<messages::Peer> _remote_peer;
  Port _listen_port;
  mutable std::mutex _connection_mutex;

 public:
  Connection(const ID id,
             networking::TransportLayer::ID transport_layer_id,
             std::shared_ptr<messages::Queue> queue,
             std::shared_ptr<tcp::socket> socket,
             std::shared_ptr<messages::Peer> remote_peer,
             const bool from_remote) :
    ::neuro::networking::Connection::Connection(id, transport_layer_id, queue),
      _socket(socket),
      _remote_peer(remote_peer),
      _listen_port(_remote_peer->port()){}

  std::shared_ptr<tcp::socket> socket();

  void read();
  void read_header();
  void read_body();

  bool send(const Buffer &message);
  const std::shared_ptr<messages::Peer> peer() const;
  const IP remote_ip() const;
  const Port remote_port() const;
  std::shared_ptr<messages::Peer> remote_peer();
  void terminate();
};
} // namespace tcp
} // namespace networking
} // namespace neuro

#endif /* NEURO_SRC_NETWORKING_CONNECTION_HPP */
