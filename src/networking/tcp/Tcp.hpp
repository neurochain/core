#ifndef NEURO_SRC_NETWORKING_TCP_TCP_HPP
#define NEURO_SRC_NETWORKING_TCP_TCP_HPP

#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <unordered_map>

#include "common/types.hpp"
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
 private:
  class ConnectionPool {
   public:
    using ID = Connection::ID;

   private:
    using ConnPtr = std::shared_ptr<tcp::Connection>;
    using Map = std::unordered_map<ID, ConnPtr>;

   public:
    using iterator = Map::iterator;

   private:
    ID _current_id;
    Tcp::ID _parent_id;
    std::shared_ptr<boost::asio::io_service> _io_context;
    Map _connections;
    mutable std::mutex _connections_mutex;

   public:
    ConnectionPool(Tcp::ID parent_id,
                   const std::shared_ptr<boost::asio::io_service> &io_context);
    ~ConnectionPool();
    std::pair<iterator, bool> insert(
        std::shared_ptr<messages::Queue> queue,
        std::shared_ptr<boost::asio::ip::tcp::socket> socket,
        std::shared_ptr<messages::Peer> remote_peer);
    std::optional<Port> connection_port(const Connection::ID id) const;
    std::size_t size() const;
    bool send(const Buffer &header_tcp, const Buffer &body_tcp);
    bool send_unicast(ID id, const Buffer &header_tcp, const Buffer &body_tcp);
    bool disconnect(ID id);
    void disconnect_all();
  };

  std::shared_ptr<boost::asio::io_service> _io_service_ptr;
  bai::tcp::resolver _resolver;
  Port _listening_port;
  bai::tcp::acceptor _acceptor;
  IP _local_ip{};
  ConnectionPool _connection_pool;
  std::shared_ptr<bai::tcp::socket> _new_socket;
  std::thread _io_context_thread;
  std::atomic<bool> _stopped;

  void new_connection(std::shared_ptr<bai::tcp::socket> socket,
                      const boost::system::error_code &error,
                      std::shared_ptr<messages::Peer> peer,
                      const bool from_remote);

  bool serialize(std::shared_ptr<messages::Message> message,
                 const ProtocolType protocol_type, Buffer *header_tcp,
                 Buffer *body_tcp);

  void start_accept();
  void accept(const boost::system::error_code &error);

 public:
  Tcp(Tcp &&) = delete;
  Tcp(const Tcp &) = delete;

  Tcp(const Port port, ID id, std::shared_ptr<messages::Queue> queue,
      std::shared_ptr<crypto::Ecc> keys);
  void connect(std::shared_ptr<messages::Peer> peer);
  bool send(std::shared_ptr<messages::Message> message,
            ProtocolType protocol_type);
  bool send_unicast(std::shared_ptr<messages::Message> message,
                    ProtocolType protocol_type);
  bool disconnect(const Connection::ID id);
  Port listening_port() const;
  IP local_ip() const;
  void terminate(const Connection::ID id);
  std::size_t peer_count() const;
  std::optional<Port> connection_port(const Connection::ID id) const;
  void stop();
  void join();
  ~Tcp();

  friend class neuro::networking::test::Tcp;
};

}  // namespace networking
}  // namespace neuro

#endif /* NEURO_SRC_NETWORKING_TCP_TCP_HPP */
