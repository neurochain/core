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
  messages::Peer _remote_peer;

  void read_header();
  void read_body(std::size_t body_size);
  std::shared_ptr<Connection> ptr() { return shared_from_this(); }
  void close();

 public:
  Connection(const ID id, messages::Queue* queue,
             const std::shared_ptr<tcp::socket>& socket,
             const messages::config::Networking &config);
  Connection(const ID id, messages::Queue* queue,
             const std::shared_ptr<tcp::socket>& socket,
             const messages::Peer& remote_peer);

  std::shared_ptr<const tcp::socket> socket() const;
  void terminate();

  void read();

  bool send(std::shared_ptr<Buffer>& message);
  const messages::Peer remote_peer() const;
  const std::optional<IP> remote_ip() const;
  const std::optional<Port> remote_port() const;
  const std::string ip() const;
  ~Connection();
};
}  // namespace tcp
}  // namespace networking
}  // namespace neuro

#endif /* NEURO_SRC_NETWORKING_CONNECTION_HPP */
