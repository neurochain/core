#ifndef NEURO_SRC_NETWORKING_TCP_TCP_HPP
#define NEURO_SRC_NETWORKING_TCP_TCP_HPP

#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <unordered_map>

#include "common/types.hpp"
#include "networking/Connection.hpp"
#include "networking/TransportLayer.hpp"

namespace neuro {
namespace messages {
class Queue;
}  // namespace messages

namespace networking {

namespace test {
class Tcp;
}  // namespace test

class Tcp : public TransportLayer {
 private:
  bool _started{false};
  boost::asio::io_service _io_service;
  bai::tcp::resolver _resolver;
  Connection::ID _current_connection_id;
  std::unordered_map<Connection::ID, tcp::Connection> _connections;
  mutable std::mutex _connection_mutex;
  std::atomic<bool> _stopping{false};
  Port _listening_port{0};
  IP _local_ip{};
  mutable std::mutex _stopping_mutex;

  void _run();
  void _stop();
  void new_connection(std::shared_ptr<bai::tcp::socket> socket,
                      const boost::system::error_code &error,
                      std::shared_ptr<messages::Peer> peer,
                      const bool from_remote);
  void accept(std::shared_ptr<bai::tcp::acceptor> acceptor, const Port port);

  bool serialize(std::shared_ptr<messages::Message> message,
                 const ProtocolType protocol_type, Buffer *header_tcp,
                 Buffer *body_tcp);

 public:
  Tcp(Tcp &&) = delete;
  Tcp(const Tcp &) = delete;

  Tcp(std::shared_ptr<messages::Queue> queue,
      std::shared_ptr<crypto::Ecc> keys);
  void accept(const Port port);
  void connect(const std::string &host, const std::string &service);
  void connect(std::shared_ptr<messages::Peer> peer);
  bool connect(const bai::tcp::endpoint host, const Port port);
  bool send(std::shared_ptr<messages::Message> message,
            ProtocolType protocol_type);
  bool send_unicast(std::shared_ptr<messages::Message> message,
                    ProtocolType protocol_type);
  bool disconnected(const Connection::ID id, std::shared_ptr<Peer> remote_peer);
  Port listening_port() const;
  IP local_ip() const;
  void terminated(const Connection::ID id);
  std::size_t peer_count() const;
  const tcp::Connection &connection(const Connection::ID id, bool &found) const;
  ~Tcp();

  friend class neuro::networking::test::Tcp;
};

}  // namespace networking
}  // namespace neuro

#endif /* NEURO_SRC_NETWORKING_TCP_TCP_HPP */
