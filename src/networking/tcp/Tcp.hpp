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
  mutable std::recursive_mutex _connections_mutex;
  const messages::config::Networking &_config;

  void new_connection_from_remote(std::shared_ptr<bai::tcp::socket> socket,
                                  const boost::system::error_code &error);

  void new_connection_local(std::shared_ptr<bai::tcp::socket> socket,
                            const boost::system::error_code &error,
                            std::shared_ptr<messages::Peer> peer);

  bool serialize(const messages::Message &message, Buffer *header_tcp,
                 Buffer *body_tcp) const;

  void start_accept();
  void accept(const boost::system::error_code &error);
  void stop();

 public:
  Tcp() = delete;
  Tcp(Tcp &&) = delete;
  Tcp(const Tcp &) = delete;

  Tcp(messages::Queue *queue, messages::Peers *peers, crypto::Ecc *keys,
      const messages::config::Networking &config);
  bool connect(std::shared_ptr<messages::Peer> peer);
  SendResult send(const messages::Message &message,
                  const Connection::ID id) const;
  SendResult send_one(const messages::Message &message) const;
  SendResult send_all(const messages::Message &message) const;

  bool reply(std::shared_ptr<messages::Message> message) const;
  Port listening_port() const;
  IP local_ip() const;
  bool terminate(Connection::ID id);
  std::shared_ptr<tcp::Connection> find(Connection::ID id) const;
  std::shared_ptr<messages::Peer> find_peer(Connection::ID id);
  void clean_old_connections(int delta);
  std::size_t peer_count() const;
  std::vector<std::shared_ptr<messages::Peer>> remote_peers() const;
  void join();
  std::string pretty_connections();
  ~Tcp();

  friend class neuro::networking::test::Tcp;
};

}  // namespace networking
}  // namespace neuro

#endif /* NEURO_SRC_NETWORKING_TCP_TCP_HPP */
