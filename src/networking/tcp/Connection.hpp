#ifndef NEURO_SRC_NETWORKING_TCP_CONNECTION_HPP
#define NEURO_SRC_NETWORKING_TCP_CONNECTION_HPP

#include <boost/asio.hpp>
#include <cmath>
#include <cstdio>
#include <iostream>

#include "common/Buffer.hpp"
#include "config.pb.h"
#include "messages.pb.h"
#include "messages/Peer.hpp"
#include "messages/Queue.hpp"
#include "networking/Connection.hpp"
#include "networking/TransportLayer.hpp"
#include "networking/tcp/HeaderPattern.hpp"

namespace neuro {
namespace messages {
class Queue;
}  // namespace messages
namespace networking {
class Tcp;

namespace tcp {

using boost::asio::ip::tcp;

class Connection : public networking::Connection,
                   public std::enable_shared_from_this<Connection> {
 private:
  Buffer _header;
  Buffer _buffer;
  std::shared_ptr<tcp::socket> _socket;
  std::shared_ptr<messages::Peer> _remote_peer;

  void read_header();
  void read_body(std::size_t body_size);
  std::shared_ptr<Connection> ptr();
  void close();

 public:
  Connection(const ID id, messages::Queue* queue,
             const std::shared_ptr<tcp::socket>& socket,
             std::shared_ptr<messages::Peer> remote_peer);

  std::shared_ptr<const tcp::socket> socket() const;
  void terminate() const;

  void read();

  bool send(std::shared_ptr<Buffer>& message);
  std::shared_ptr<messages::Peer> remote_peer() const;
  const std::optional<IP> remote_ip() const;
  const std::optional<Port> remote_port() const;
  const std::optional<Port> local_port() const;
  const std::string ip() const;
  ~Connection();
};
}  // namespace tcp
}  // namespace networking
}  // namespace neuro

#endif /* NEURO_SRC_NETWORKING_CONNECTION_HPP */
