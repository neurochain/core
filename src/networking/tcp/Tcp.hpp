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
    Map _connections;
    mutable std::mutex _connections_mutex;

   public:
    ConnectionPool(Tcp::ID parent_id) : _current_id(0), _parent_id(parent_id) {}

    std::pair<iterator, bool> insert(
        std::shared_ptr<messages::Queue> queue,
        std::shared_ptr<boost::asio::ip::tcp::socket> socket,
        std::shared_ptr<messages::Peer> remote_peer, const bool from_remote) {
      std::lock_guard<std::mutex> lock_queue(_connections_mutex);

      auto connection = std::make_shared<tcp::Connection>(
          _current_id, _parent_id, queue, socket, remote_peer, from_remote);
      return _connections.insert(std::make_pair(_current_id++, connection));
    }

    std::shared_ptr<tcp::Connection> find(const Connection::ID id) const {
      std::shared_ptr<tcp::Connection> ans;
      std::lock_guard<std::mutex> lock_queue(_connections_mutex);
      auto got = _connections.find(id);
      if (got != _connections.end()) {
        ans = got->second;
      } else {
        LOG_ERROR << this << " " << __LINE__ << " Connection not found";
      }
      return ans;
    }

    bool erase(ID id) {
      std::lock_guard<std::mutex> lock_queue(_connections_mutex);
      auto got = _connections.find(id);
      if (got == _connections.end()) {
        LOG_ERROR << this << " " << __LINE__ << " Connection not found " << id;
        return false;
      }
      _connections.erase(got);
      return true;
    }

    std::size_t size() const {
      std::lock_guard<std::mutex> lock_queue(_connections_mutex);
      return _connections.size();
    }

    bool send(const Buffer &header_tcp, const Buffer &body_tcp) {
      std::lock_guard<std::mutex> lock_queue(_connections_mutex);
      bool res = true;
      for (auto &it : _connections) {
        auto &connection = it.second;
        res &= connection->send(header_tcp);
        res &= connection->send(body_tcp);
      }
      return res;
    }

    bool send_unicast(ID id, const Buffer &header_tcp, const Buffer &body_tcp) {
      std::lock_guard<std::mutex> lock_queue(_connections_mutex);
      auto got = _connections.find(id);
      if (got == _connections.end()) {
        return false;
      }
      got->second->send(header_tcp);
      got->second->send(body_tcp);
      return true;
    }

    bool disconnect(ID id) {
      std::lock_guard<std::mutex> lock_queue(_connections_mutex);
      auto got = _connections.find(id);
      if (got == _connections.end()) {
        LOG_WARNING << __LINE__ << " Connection not found";
        return false;
      }
      _connections.erase(got);
      return true;
    }
  };

  bool _started{false};
  boost::asio::io_service _io_service;
  bai::tcp::resolver _resolver;
  std::atomic<bool> _stopping{false};
  Port _listening_port{0};
  IP _local_ip{};
  mutable std::mutex _stopping_mutex;
  ConnectionPool _connection_pool;

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

  Tcp(ID id, std::shared_ptr<messages::Queue> queue,
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
  void terminate(const Connection::ID id);
  std::size_t peer_count() const;
  std::shared_ptr<tcp::Connection> connection(const Connection::ID id) const;
  ~Tcp();

  friend class neuro::networking::test::Tcp;
};

}  // namespace networking
}  // namespace neuro

#endif /* NEURO_SRC_NETWORKING_TCP_TCP_HPP */
