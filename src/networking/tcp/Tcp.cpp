#include <stdio.h>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <thread>
#include <tuple>

#include "common/logger.hpp"
#include "crypto/Ecc.hpp"
#include "networking/Networking.hpp"
#include "networking/tcp/HeaderPattern.hpp"
#include "networking/tcp/Tcp.hpp"

namespace neuro {
namespace networking {
using namespace std::chrono_literals;

Tcp::ConnectionPool::ConnectionPool(
    const Tcp::ID parent_id,
    const std::shared_ptr<boost::asio::io_context> &io_context)
    : _current_id(0), _parent_id(parent_id), _io_context(io_context) {}

std::pair<Tcp::ConnectionPool::iterator, bool> Tcp::ConnectionPool::insert(
    std::shared_ptr<messages::Queue> queue,
    std::shared_ptr<boost::asio::ip::tcp::socket> socket,
    std::shared_ptr<messages::Peer> remote_peer) {
  std::lock_guard<std::mutex> lock_queue(_connections_mutex);

  auto connection = std::make_shared<tcp::Connection>(
      _current_id, _parent_id, _io_context, queue, socket, remote_peer);
  return _connections.insert(std::make_pair(_current_id++, connection));
}

std::optional<Port> Tcp::ConnectionPool::connection_port(
    const Connection::ID id) const {
  std::optional<Port> ans;
  std::lock_guard<std::mutex> lock_queue(_connections_mutex);
  auto got = _connections.find(id);
  if (got != _connections.end()) {
    ans = got->second->listen_port();
  } else {
    LOG_ERROR << this << " " << __LINE__ << " Connection not found";
  }
  return ans;
}

std::size_t Tcp::ConnectionPool::size() const {
  std::lock_guard<std::mutex> lock_queue(_connections_mutex);
  return _connections.size();
}

bool Tcp::ConnectionPool::send(std::shared_ptr<Buffer> &header_tcp,
                               std::shared_ptr<Buffer> &body_tcp) {
  std::lock_guard<std::mutex> lock_queue(_connections_mutex);
  bool res = true;
  for (auto &it : _connections) {
    auto &connection = it.second;
    res &= connection->send(header_tcp);
    res &= connection->send(body_tcp);
  }
  return res;
}

bool Tcp::ConnectionPool::send_unicast(ID id,
                                       std::shared_ptr<Buffer> &header_tcp,
                                       std::shared_ptr<Buffer> &body_tcp) {
  std::lock_guard<std::mutex> lock_queue(_connections_mutex);
  auto got = _connections.find(id);
  if (got == _connections.end()) {
    return false;
  }
  got->second->send(header_tcp);
  got->second->send(body_tcp);
  return true;
}

bool Tcp::ConnectionPool::disconnect(ID id) {
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

void Tcp::ConnectionPool::disconnect_all() {
  std::lock_guard<std::mutex> lock_queue(_connections_mutex);
  for (auto &it : _connections) {
    it.second->close();
  }
  _connections.clear();
}

Tcp::ConnectionPool::~ConnectionPool() { disconnect_all(); }

void Tcp::start_accept() {
  if (!_stopped) {
    _new_socket = std::make_shared<bai::tcp::socket>(*_io_context_ptr);
    _acceptor.async_accept(
        *_new_socket.get(),
        boost::bind(&Tcp::accept, this, boost::asio::placeholders::error));
  }
}

void Tcp::accept(const boost::system::error_code &error) {
  if (!error) {
    const auto remote_endpoint =
        _new_socket->remote_endpoint().address().to_string();

    auto peer = std::make_shared<messages::Peer>();
    peer->set_endpoint(remote_endpoint);
    peer->set_port(_new_socket->remote_endpoint().port());
    peer->set_status(messages::Peer::CONNECTING);
    peer->set_transport_layer_id(_id);

    this->new_connection(_new_socket, error, peer, true);
  } else {
    LOG_WARNING << "Rejected connection.";
  }
  _new_socket.reset();
  start_accept();
}  // namespace networking

void Tcp::init_signals() {
#if defined(SIGINT)
  _signals.add(SIGINT);
  LOG_INFO << "Tcp transport layer will listen for SIGINT";
#endif
#if defined(SIGTERM)
  _signals.add(SIGTERM);
  LOG_INFO << "Tcp transport layer will listen for SIGTERM";
#endif
#if defined(SIGQUIT)
  _signals.add(SIGQUIT);
  LOG_INFO << "Tcp transport layer will listen for SIGQUIT";
#endif
  _signals.async_wait([this](const boost::system::error_code &error,
                             int sig_num) {
    if (!error) {
      LOG_INFO << "TCP transport layer stopped by signal code " << sig_num;
      stop();
    } else {
      LOG_INFO << "Signalset aborted with code " << sig_num << ": " << error;
    }
  });
  LOG_INFO << "Signal handler set for TCP transport layer";
}

Tcp::Tcp(const Port port, ID id, std::shared_ptr<messages::Queue> queue,
         std::shared_ptr<crypto::Ecc> keys)
    : TransportLayer(id, queue, keys),
      _stopped(false),
      _io_context_ptr(std::make_shared<boost::asio::io_context>()),
      _resolver(*_io_context_ptr),
      _listening_port(port),
      _acceptor(*_io_context_ptr.get(),
                bai::tcp::endpoint(bai::tcp::v4(), _listening_port)),
      _connection_pool(id, _io_context_ptr),
      _signals(*_io_context_ptr) {
  init_signals();
  while (!_acceptor.is_open()) {
    std::this_thread::yield();
    LOG_DEBUG << "Waiting for acceptor to be open";
  }
  start_accept();
  _io_context_thread = std::thread([this]() { this->_io_context_ptr->run(); });
}

bool Tcp::connect(std::shared_ptr<messages::Peer> peer) {
  if (_stopped) {
    return false;
  }

  bai::tcp::resolver resolver(*_io_context_ptr);
  bai::tcp::resolver::query query(peer->endpoint(),
                                  std::to_string(peer->port()));
  bai::tcp::resolver::iterator endpoint_iterator = resolver.resolve(query);

  auto socket = std::make_shared<bai::tcp::socket>(*_io_context_ptr);
  auto handler = [this, socket, peer](boost::system::error_code error,
                                      bai::tcp::resolver::iterator iterator) {
    this->new_connection(socket, error, peer, false);
  };
  boost::asio::async_connect(*socket, endpoint_iterator, handler);
  return true;
}

Port Tcp::listening_port() const { return _listening_port; }
IP Tcp::local_ip() const { return _local_ip; }

void Tcp::new_connection(std::shared_ptr<bai::tcp::socket> socket,
                         const boost::system::error_code &error,
                         std::shared_ptr<messages::Peer> peer,
                         const bool from_remote) {
  auto message = std::make_shared<messages::Message>();
  auto msg_header = message->mutable_header();
  auto msg_peer = msg_header->mutable_peer();
  auto msg_body = message->add_bodies();

  if (!error) {
    auto conn_insertion = _connection_pool.insert(_queue, socket, peer);
    auto &connection_ptr = conn_insertion.first->second;
    peer->set_connection_id(connection_ptr->id());

    auto connection_ready = msg_body->mutable_connection_ready();

    connection_ready->set_from_remote(from_remote);

    msg_peer->CopyFrom(*peer);
    _queue->publish(message);
    connection_ptr->read();
  } else {
    LOG_WARNING << "Could not create new connection to " << *peer << " due to "
                << error.message();

    msg_body->mutable_connection_closed();
    msg_peer->CopyFrom(*peer);
    _queue->publish(message);
  }
}

std::optional<Port> Tcp::connection_port(const Connection::ID id) const {
  return _connection_pool.connection_port(id);
}

void Tcp::terminate(const Connection::ID id) {
  _connection_pool.disconnect(id);
}

bool Tcp::serialize(std::shared_ptr<messages::Message> message,
                    const ProtocolType protocol_type, Buffer *header_tcp,
                    Buffer *body_tcp) {
  LOG_DEBUG << this << " Before reinterpret and signing";
  auto header_pattern =
      reinterpret_cast<tcp::HeaderPattern *>(header_tcp->data());
  messages::to_buffer(*message, body_tcp);

  if (body_tcp->size() > (1 << (8 * sizeof(tcp::HeaderPattern::size)))) {
    LOG_ERROR << "Message is too big (" << body_tcp->size() << ")";
    return false;
  }

  header_pattern->size = body_tcp->size();
  _keys->sign(body_tcp->data(), body_tcp->size(),
              reinterpret_cast<uint8_t *>(&header_pattern->signature));
  header_pattern->type = protocol_type;

  return true;
}

bool Tcp::send(std::shared_ptr<messages::Message> message,
               ProtocolType protocol_type) {
  if (_connection_pool.size() == 0) {
    LOG_ERROR << "Could not send message because there is no connection";
    return false;
  }

  if (_stopped) {
    return false;
  }
  auto header_tcp =
      std::make_shared<Buffer>(sizeof(networking::tcp::HeaderPattern), 0);
  auto body_tcp = std::make_shared<Buffer>();

  std::cout << "\033[1;34mSending message: >>" << *message << "<<\033[0m\n";
  serialize(message, protocol_type, header_tcp.get(), body_tcp.get());

  return _connection_pool.send(header_tcp, body_tcp);
}

bool Tcp::send_unicast(std::shared_ptr<messages::Message> message,
                       ProtocolType protocol_type) {
  if (_stopped) {
    return false;
  }
  assert(message->header().has_peer());
  auto header_tcp =
      std::make_shared<Buffer>(sizeof(networking::tcp::HeaderPattern), 0);
  auto body_tcp = std::make_shared<Buffer>();
  LOG_DEBUG << "\033[1;34mSending unicast : >>" << *message << "<<\033[0m";
  serialize(message, protocol_type, header_tcp.get(), body_tcp.get());
  return _connection_pool.send_unicast(message->header().peer().connection_id(),
                                       header_tcp, body_tcp);
}

std::size_t Tcp::peer_count() const { return _connection_pool.size(); }

bool Tcp::disconnect(const Connection::ID id) {
  return _connection_pool.disconnect(id);
}

void Tcp::stop() {
  if (!_stopped) {
    _stopped = true;
    _signals.cancel();
    _io_context_ptr->post([this]() { _acceptor.close(); });
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
