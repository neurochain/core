#ifndef NEURO_SRC_NETWORKING_TCP_TCP_HPP
#define NEURO_SRC_NETWORKING_TCP_TCP_HPP

#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <unordered_map>

#include "common/Buffer.hpp"
#include "common/types.hpp"
#include "config.pb.h"
#include "crypto/Ecc.hpp"
#include "messages/Peer.hpp"
#include "messages/Peers.hpp"
#include "messages/Queue.hpp"
#include "networking/Connection.hpp"
#include "networking/TransportLayer.hpp"
#include "networking/tcp/Connection.hpp"

namespace neuro {
namespace messages {
class Queue;
}  // namespace messages

namespace networking {

namespace test {
class Tcp;
}  // namespace test

class Tcp : public TransportLayer {
 public:
  using ID = tcp::Connection::ID;

 private:
  using ConnectionById =
      std::unordered_map<ID, std::shared_ptr<tcp::Connection>>;

  ID _current_id{0};
  std::atomic<bool> _stopped;
  Port _listening_port;
  boost::asio::io_context _io_context;
  bai::tcp::resolver _resolver;
  bai::tcp::acceptor _acceptor;
  std::shared_ptr<bai::tcp::socket> _new_socket;
  std::thread _io_context_thread;
  ConnectionById _connections;
  mutable std::mutex _connections_mutex;
  const messages::config::Networking &_config;

  void new_connection_from_remote(std::shared_ptr<bai::tcp::socket> socket,
                                  const boost::system::error_code &error);

  void new_connection_local(std::shared_ptr<bai::tcp::socket> socket,
                            const boost::system::error_code &error,
                            messages::Peer *peer);

  bool serialize(std::shared_ptr<messages::Message> message, Buffer *header_tcp,
                 Buffer *body_tcp) const;

  void start_accept();
  void accept(const boost::system::error_code &error);

 public:
  Tcp() = delete;
  Tcp(Tcp &&) = delete;
  Tcp(const Tcp &) = delete;

  Tcp(messages::Queue *queue, messages::Peers *peers, crypto::Ecc *keys,
      const messages::config::Networking &config);
  bool connect(messages::Peer *peer);
  SendResult send(std::shared_ptr<messages::Message> message) const;
  bool send_unicast(std::shared_ptr<messages::Message> message) const;
  Port listening_port() const;
  IP local_ip() const;
  bool terminate(Connection::ID id);
  std::optional<tcp::Connection *> find(Connection::ID id) const;
  std::optional<messages::Peer *> find_peer(Connection::ID id);
  void clean_old_connections(int delta);
  std::size_t peer_count() const;
  void stop();
  void join();
  ~Tcp();

  friend class neuro::networking::test::Tcp;
};

}  // namespace networking
}  // namespace neuro

#endif /* NEURO_SRC_NETWORKING_TCP_TCP_HPP */
