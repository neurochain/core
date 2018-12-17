#include <stdio.h>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <thread>
#include <tuple>

#include "common/logger.hpp"
#include "networking/Networking.hpp"
#include "networking/tcp/InboundConnection.hpp"
#include "networking/tcp/OutboundConnection.hpp"
#include "networking/tcp/Tcp.hpp"

namespace neuro {
namespace networking {
using namespace std::chrono_literals;

Tcp::ConnectionPool::ConnectionPool(
    std::size_t max_size,
    const std::shared_ptr<boost::asio::io_context>& io_context)
    : _io_context(io_context), _max_size(max_size) {}

bool Tcp::ConnectionPool::insert(
    const RemoteKey& id,
    const std::shared_ptr<tcp::Connection>& paired_connection) {
  std::lock_guard<std::mutex> lock_queue(_connections_mutex);
  if (_connections.size() >= _max_size) {
    return false;
  }
  return _connections.insert(std::make_pair(id, paired_connection)).second;
}

std::size_t Tcp::ConnectionPool::size() const {
  std::lock_guard<std::mutex> lock_queue(_connections_mutex);
  return _connections.size();
}

bool Tcp::ConnectionPool::send(const messages::Message& message) {
  std::lock_guard<std::mutex> lock_queue(_connections_mutex);
  bool res = true;
  for (auto& it : _connections) {
    auto& connection = it.second;
    res &= connection->send(message);
  }
  return res;
}

bool Tcp::ConnectionPool::is_full() const {
  std::lock_guard<std::mutex> lock_queue(_connections_mutex);
  return _connections.size() >= _max_size;
}

bool Tcp::ConnectionPool::send_unicast(const ID& id,
                                       const messages::Message& message) {
  std::lock_guard<std::mutex> lock_queue(_connections_mutex);
  auto got = _connections.find(id);
  if (got == _connections.end()) {
    return false;
  }
  got->second->send(message);
  return true;
}

bool Tcp::ConnectionPool::disconnect(const ID& id) {
  std::lock_guard<std::mutex> lock_queue(_connections_mutex);
  auto got = _connections.find(id);
  if (got == _connections.end()) {
    LOG_WARNING << __LINE__ << " Connection not found";
    return false;
  }
  got->second->close();
  _connections.erase(got);
  return true;
}

bool Tcp::ConnectionPool::disconnect_all() {
  std::lock_guard<std::mutex> lock_queue(_connections_mutex);
  for (auto& it : _connections) {
    it.second->close();
  }
  _connections.clear();
  return true;
}

Tcp::ConnectionPool::~ConnectionPool() { disconnect_all(); }

void Tcp::start_accept() {
  if (!_stopped) {
    _new_socket = std::make_shared<bai::tcp::socket>(*_io_context);
    _acceptor.async_accept(
        *_new_socket.get(),
        boost::bind(&Tcp::accept, this, boost::asio::placeholders::error));
  }
}

void Tcp::accept(const boost::system::error_code& error) {
  if (!error) {
    const auto remote_endpoint =
        _new_socket->remote_endpoint().address().to_string();

    auto peer = std::make_shared<messages::Peer>();
    peer->set_endpoint(remote_endpoint);
    peer->set_port(_new_socket->remote_endpoint().port());

    this->new_inbound_connection(_new_socket, error, peer);
  } else {
    LOG_WARNING << "Rejected connection.";
  }
  _new_socket.reset();
  start_accept();
}

Tcp::Tcp(const Port port, std::shared_ptr<messages::Queue> queue,
         std::shared_ptr<crypto::Ecc> keys, std::size_t max_size)
    : TransportLayer(queue, keys),
      _stopped(false),
      _io_context(std::make_shared<boost::asio::io_context>()),
      _resolver(*_io_context),
      _listening_port(port),
      _acceptor(*_io_context.get(),
                bai::tcp::endpoint(bai::tcp::v4(), _listening_port)),
      _connection_pool(max_size, _io_context) {
  while (!_acceptor.is_open()) {
    std::this_thread::yield();
    LOG_DEBUG << "Waiting for acceptor to be open";
  }
  start_accept();
  _io_context_thread = std::thread([this]() { this->_io_context->run(); });
}

bool Tcp::connect(std::shared_ptr<messages::Peer> peer) {
  if (_stopped) {
    return false;
  }

  bai::tcp::resolver resolver(*_io_context);
  bai::tcp::resolver::query query(peer->endpoint(),
                                  std::to_string(peer->port()));
  bai::tcp::resolver::iterator endpoint_iterator = resolver.resolve(query);

  auto socket = std::make_shared<bai::tcp::socket>(*_io_context);
  auto handler = [this, socket, peer](boost::system::error_code error,
                                      bai::tcp::resolver::iterator iterator) {
    this->new_outbound_connection(socket, error, peer);
  };
  boost::asio::async_connect(*socket, endpoint_iterator, handler);
  return true;
}

Port Tcp::listening_port() const { return _listening_port; }

IP Tcp::local_ip() const { return _local_ip; }

std::shared_ptr<messages::Message> Tcp::build_new_connection_message() const {
  auto message = std::make_shared<messages::Message>();
  auto msg_header = message->mutable_header();
  messages::fill_header(msg_header);
  auto msg_body = message->add_bodies();
  msg_body->mutable_connection_ready();
  return message;
}

std::shared_ptr<messages::Message> Tcp::build_disconnection_message() const {
  auto message = std::make_shared<messages::Message>();
  auto msg_header = message->mutable_header();
  messages::fill_header(msg_header);
  auto msg_body = message->add_bodies();
  msg_body->mutable_connection_closed();
  return message;
}

void Tcp::new_outbound_connection(
    const std::shared_ptr<bai::tcp::socket>& socket,
    const boost::system::error_code& error,
    std::shared_ptr<messages::Peer> peer) {
  if (!error) {
    LOG_WARNING << "Could not create new connection to " << *peer << " due to "
                << error.message();
    return;
  }
  std::shared_ptr<tcp::OutboundConnection> new_connection;
  new_connection = std::make_shared<tcp::OutboundConnection>(
      _keys, _queue, socket, peer,
      [this](const std::shared_ptr<tcp::Connection>& paired_connection,
             const RemoteKey& id) {
        if (this->_connection_pool.insert(id, paired_connection)) {
          this->_queue->publish(this->build_new_connection_message());
        }
      },
      [this](const RemoteKey& id) {
        if (this->_connection_pool.disconnect(id)) {
          this->_queue->publish(this->build_disconnection_message());
        }
      });
}

void Tcp::new_inbound_connection(
    const std::shared_ptr<bai::tcp::socket>& socket,
    const boost::system::error_code& error,
    std::shared_ptr<messages::Peer> peer) {
  if (!error) {
    std::shared_ptr<tcp::InboundConnection> new_connection;
    new_connection = std::make_shared<tcp::InboundConnection>(
        _keys, _queue, socket,
        [this](const std::shared_ptr<tcp::Connection>& paired_connection,
               const RemoteKey& id) {
          if (!this->_connection_pool.insert(id, paired_connection)) {
            this->_queue->publish(this->build_new_connection_message());
          }
        },
        [this](const RemoteKey& id) {
          if (this->_connection_pool.disconnect(id)) {
            this->_queue->publish(this->build_disconnection_message());
          }
        });
  } else {
    LOG_WARNING << "Could not create new connection to " << *peer << " due to "
                << error.message();
  }
}

void Tcp::terminate(const RemoteKey& id) { _connection_pool.disconnect(id); }

bool Tcp::send(const messages::Message& message) {
  if (_connection_pool.size() == 0) {
    LOG_ERROR << "Could not send message because there is no connection";
    return false;
  }

  if (_stopped) {
    LOG_ERROR << "Could not send message because stopping.";
    return false;
  }
  return _connection_pool.send(message);
}

bool Tcp::send_unicast(const RemoteKey& id,
                       const messages::Message& message) {
  if (_stopped) {
    return false;
  }
  auto header_tcp =
      std::make_shared<Buffer>(sizeof(networking::tcp::HeaderPattern), 0);
  auto body_tcp = std::make_shared<Buffer>();
  return _connection_pool.send_unicast(id, message);
}

std::size_t Tcp::peer_count() const { return _connection_pool.size(); }

bool Tcp::disconnect(const RemoteKey& key) {
  return _connection_pool.disconnect(key);
}

void Tcp::stop() {
  if (!_stopped) {
    _stopped = true;
    _io_context->post([this]() { _acceptor.close(); });
    _connection_pool.disconnect_all();
    join();
  }
  LOG_DEBUG << this << " TCP stopped";
}

void Tcp::join() {
  if (_io_context_thread.joinable()) {
    _io_context_thread.join();
  }
  LOG_DEBUG << this << " TCP joined";
}

Tcp::~Tcp() {
  stop();
  LOG_DEBUG << this << " TCP killed";
}

}  // namespace networking
}  // namespace neuro
