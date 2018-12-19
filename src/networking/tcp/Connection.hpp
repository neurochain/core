#ifndef NEURO_SRC_NETWORKING_TCP_CONNECTION_HPP
#define NEURO_SRC_NETWORKING_TCP_CONNECTION_HPP

#include <stdio.h>
#include <boost/asio.hpp>
#include <boost/bind.hpp>
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
 public:
  using PairingCallback = std::function<void(
      const std::shared_ptr<Connection>& paired_connection, const Buffer& id)>;
  using UnpairingCallback = std::function<void(const Buffer& id)>;

 protected:
  Buffer _header;
  Buffer _buffer;
  std::shared_ptr<tcp::socket> _socket;
  std::shared_ptr<crypto::Ecc> _keys;
  std::optional<crypto::EccPub> _remote_pub_key;
  UnpairingCallback _unpairing_callback;

  class Packet {
   public:
    static bool serialize(const crypto::Ecc& keys,
                          const messages::Message& message, Buffer& header_tcp,
                          Buffer& body_tcp);
    static std::shared_ptr<messages::Message> deserialize(
        const std::optional<crypto::EccPub>& pub_key, const Buffer& header_tcp,
        const Buffer& body_tcp);
  };

protected:
 std::shared_ptr<Connection> ptr() { return shared_from_this(); }

private:
 void terminate();
 void read_body(std::size_t body_size);
 messages::Peer build_peer(const IP& ip, Port port,
                           const crypto::EccPub& ecc_pub) const;
 messages::Peer build_remote_peer() const;
 messages::Peer build_local_peer() const;
 bool send(const std::shared_ptr<Buffer>& header_tcp,
           const std::shared_ptr<Buffer>& body_tcp);

protected:
 Connection(const std::shared_ptr<crypto::Ecc>& keys,
            const std::shared_ptr<messages::Queue>& queue,
            const std::shared_ptr<tcp::socket>& socket,
            UnpairingCallback unpairing_callback);
 void read_header();
 virtual void read_handshake_message_body(PairingCallback pairing_callback,
                                          std::size_t body_size) = 0;

public:
 void start(PairingCallback pairing_callback);
 std::shared_ptr<const tcp::socket> socket() const;
 bool send(const messages::Message& message);
 IP remote_ip() const;
 Port remote_port() const;
 IP local_ip() const;
 Port local_port() const;
 void close();
 virtual ~Connection();
};

}  // namespace tcp
}  // namespace networking
}  // namespace neuro

#endif /* NEURO_SRC_NETWORKING_CONNECTION_HPP */
