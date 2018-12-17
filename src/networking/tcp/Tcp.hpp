#ifndef NEURO_SRC_NETWORKING_TCP_TCP_HPP
#define NEURO_SRC_NETWORKING_TCP_TCP_HPP

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
    using ID = TransportLayer::RemoteKey;

   private:
    using ConnectedPeers =
        std::unordered_map<ID, std::shared_ptr<tcp::Connection>>;

   public:
    using iterator = ConnectedPeers::iterator;

   private:
    std::shared_ptr<boost::asio::io_context> _io_context;
    ConnectedPeers _connections;
    std::size_t _max_size;
    mutable std::mutex _connections_mutex;

   public:
    ConnectionPool(std::size_t max_size,
                   const std::shared_ptr<boost::asio::io_context>& io_context);
    ~ConnectionPool();
    bool insert(const ID& id,
                const std::shared_ptr<tcp::Connection>& paired_connection);
    std::size_t size() const;
    bool send(const messages::Message& message);
    bool send_unicast(const ID& id, const messages::Message& message);
    bool disconnect(const ID& id);
    bool disconnect_all();
    bool is_full() const;
  };

  using KnownRemotes = std::set<messages::Peer>;

  std::atomic<bool> _stopped;
  std::shared_ptr<boost::asio::io_context> _io_context;
  bai::tcp::resolver _resolver;
  Port _listening_port;
  bai::tcp::acceptor _acceptor;
  IP _local_ip{};
  ConnectionPool _connection_pool;
  KnownRemotes _know_remotes;
  std::shared_ptr<bai::tcp::socket> _new_socket;
  std::thread _io_context_thread;

  std::shared_ptr<messages::Message> build_new_connection_message() const;
  std::shared_ptr<messages::Message> build_disconnection_message() const;
  void new_outbound_connection(const std::shared_ptr<bai::tcp::socket>& socket,
                               const boost::system::error_code& error,
                               std::shared_ptr<messages::Peer> peer);

  void new_inbound_connection(const std::shared_ptr<bai::tcp::socket>& socket,
                              const boost::system::error_code& error,
                              std::shared_ptr<messages::Peer> peer);

  void start_accept();
  void accept(const boost::system::error_code& error);
  void terminate(const RemoteKey& id);
  bool disconnect(const RemoteKey& id);

 public:
  Tcp(Tcp&&) = delete;
  Tcp(const Tcp&) = delete;
  Tcp(Port port, std::shared_ptr<messages::Queue> queue,
      std::shared_ptr<crypto::Ecc> keys, std::size_t max_size);
  bool send(const messages::Message& message);
  bool send_unicast(const RemoteKey& id,
                    const messages::Message& message);
  Port listening_port() const;
  IP local_ip() const;
  std::size_t peer_count() const;
  void stop();
  void join();
  ~Tcp();

  bool connect(std::shared_ptr<messages::Peer> peer);
  friend class neuro::networking::test::Tcp;
};

}  // namespace networking
}  // namespace neuro

#endif /* NEURO_SRC_NETWORKING_TCP_TCP_HPP */
