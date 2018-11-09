#ifndef NEURO_SRC_NETWORKING_TCP_CONNECTION_HPP
#define NEURO_SRC_NETWORKING_TCP_CONNECTION_HPP

#include <stdio.h>
#include <boost/asio.hpp>
#include <cmath>
#include <iostream>

#include "messages.pb.h"
#include "networking/Connection.hpp"
#include "networking/TransportLayer.hpp"
#include "networking/tcp/HeaderPattern.hpp"

namespace neuro {
namespace messages {
class Queue;
}  // namespace messages
namespace networking {
namespace tcp {
using boost::asio::ip::tcp;

class Connection : public networking::Connection,
                   public std::enable_shared_from_this<Connection> {
 private:
  Buffer _header;
  Buffer _buffer;
  std::shared_ptr<tcp::socket> _socket;
  std::shared_ptr<messages::Peer> _remote_peer;
  Port _listen_port;

  void terminate();
  void read_header();
  void read_body(std::size_t body_size);
  std::shared_ptr<Connection> ptr() { return shared_from_this(); }

 public:
  Connection(const ID id, networking::TransportLayer::ID transport_layer_id,
             std::shared_ptr<messages::Queue> queue,
             std::shared_ptr<tcp::socket> socket,
             std::shared_ptr<messages::Peer> remote_peer)
      : ::neuro::networking::Connection::Connection(id, transport_layer_id,
                                                    queue),
        _header(sizeof(HeaderPattern), 0),
        _buffer(128, 0),
        _socket(socket),
        _remote_peer(remote_peer),
        _listen_port(_remote_peer->port()) {}

  std::shared_ptr<const tcp::socket> socket() const;

  void read();

  bool send(const Buffer &message);
  std::shared_ptr<const messages::Peer> remote_peer() const;
  const IP remote_ip() const;
  const Port remote_port() const;
  const Port listen_port() const;
  void close();
  ~Connection();
};
}  // namespace tcp
}  // namespace networking
}  // namespace neuro

#endif /* NEURO_SRC_NETWORKING_CONNECTION_HPP */
