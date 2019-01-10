#include <stdio.h>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <thread>
#include <tuple>
#include <utility>

#include "common/logger.hpp"
#include "crypto/Ecc.hpp"
#include "networking/Networking.hpp"
#include "networking/tcp/HeaderPattern.hpp"
#include "networking/tcp/Tcp.hpp"

namespace neuro {
namespace networking {
using namespace std::chrono_literals;

// Tcp::ConnectionPool::ConnectionPool(
//     const Tcp::ID parent_id,
//     const std::shared_ptr<boost::asio::io_context> &io_context)
//     : _current_id(0), _parent_id(parent_id), _io_context(io_context) {}

// std::pair<Tcp::ConnectionPool::iterator, bool> Tcp::ConnectionPool::insert(
//     messages::Queue* queue,
//     std::shared_ptr<boost::asio::ip::tcp::socket> socket,
//     std::shared_ptr<messages::Peer> remote_peer) {
//   std::lock_guard<std::mutex> lock_queue(_connections_mutex);

//   auto connection = std::make_shared<tcp::Connection>(
//       _current_id, _parent_id, queue, socket, remote_peer);
//   return _connections.insert(std::make_pair(_current_id++, connection));
// }

// std::optional<Port> Tcp::ConnectionPool::connection_port(
//     const Connection::ID id) const {
//   std::optional<Port> ans;
//   std::lock_guard<std::mutex> lock_queue(_connections_mutex);
//   auto got = _connections.find(id);
//   if (got != _connections.end()) {
//     ans = got->second->listen_port();
//   } else {
//     LOG_ERROR << this << " " << __LINE__ << " Connection not found";
//   }
//   return ans;
// }

// std::size_t Tcp::ConnectionPool::size() const {
//   std::lock_guard<std::mutex> lock_queue(_connections_mutex);
//   return _connections.size();
// }

// bool Tcp::ConnectionPool::send(std::shared_ptr<Buffer> &header_tcp,
//                                std::shared_ptr<Buffer> &body_tcp) {
//   std::lock_guard<std::mutex> lock_queue(_connections_mutex);
//   bool res = true;
//   for (auto &it : _connections) {
//     auto &connection = it.second;
//     res &= connection->send(header_tcp);
//     res &= connection->send(body_tcp);
//   }
//   return res;
// }

// bool Tcp::ConnectionPool::send_unicast(ID id,
//                                        std::shared_ptr<Buffer> &header_tcp,
//                                        std::shared_ptr<Buffer> &body_tcp) {
//   std::lock_guard<std::mutex> lock_queue(_connections_mutex);
//   auto got = _connections.find(id);
//   if (got == _connections.end()) {
//     return false;
//   }
//   got->second->send(header_tcp);
//   got->second->send(body_tcp);
//   return true;
// }

// bool Tcp::ConnectionPool::disconnect(const ID id) {
//   std::lock_guard<std::mutex> lock_queue(_connections_mutex);
//   auto got = _connections.find(id);
//   if (got == _connections.end()) {
//     LOG_WARNING << __LINE__ << " Connection not found";
//     return false;
//   }
//   got->second->close();
//   _connections.erase(got);
//   return true;
// }

// bool Tcp::ConnectionPool::disconnect_all() {
//   std::lock_guard<std::mutex> lock_queue(_connections_mutex);
//   for (auto &it : _connections) {
//     it.second->close();
//   }
//   _connections.clear();
//   return true;
// }

// Tcp::ConnectionPool::~ConnectionPool() { disconnect_all(); }

void Tcp::start_accept() {
  if (!_stopped) {
    _new_socket = std::make_shared<bai::tcp::socket>(_io_context);
    _acceptor.async_accept(
        *_new_socket.get(),
        boost::bind(&Tcp::accept, this, boost::asio::placeholders::error));
  }
}

void Tcp::accept(const boost::system::error_code &error) {
  if (error) {
    const auto remote_endpoint =
        _new_socket->remote_endpoint().address().to_string();

    auto peer = _peers->add_peers();
    peer->set_endpoint(remote_endpoint);
    peer->set_port(_new_socket->remote_endpoint().port());

    this->new_connection(_new_socket, error, peer, true);
  } else {
    LOG_WARNING << "Rejected connection";
  }
  _new_socket.reset();  // TODO why?
  start_accept();
}

Tcp::Tcp(const Port port, messages::Queue *queue, messages::Peers *peers,
         std::shared_ptr<crypto::Ecc> keys)
    : TransportLayer(queue, peers, keys),
      _stopped(false),
      _listening_port(port),
      _io_context(),
      _resolver(_io_context),
      _acceptor(_io_context,
                bai::tcp::endpoint(bai::tcp::v4(), _listening_port)),
      _peers(peers) {
  while (!_acceptor.is_open()) {
    std::this_thread::yield();
    LOG_DEBUG << "Waiting for acceptor to be open";
  }
  start_accept();
  _io_context_thread = std::thread([this]() { this->_io_context.run(); });
}

bool Tcp::connect(messages::Peer * peer) {
  if (_stopped) {
    return false;
  }

  bai::tcp::resolver resolver(_io_context);
  bai::tcp::resolver::query query(peer->endpoint(),
                                  std::to_string(peer->port()));
  bai::tcp::resolver::iterator endpoint_iterator = resolver.resolve(query);

  auto socket = std::make_shared<bai::tcp::socket>(_io_context);
  auto handler = [this, socket, peer](boost::system::error_code error,
                                      bai::tcp::resolver::iterator iterator) {
    this->new_connection(socket, error, peer, false);
  };
  boost::asio::async_connect(*socket, endpoint_iterator, handler);
  return true;
}

Port Tcp::listening_port() const { return _listening_port; }

void Tcp::new_connection(std::shared_ptr<bai::tcp::socket> socket,
                         const boost::system::error_code &error,
                         messages::Peer* peer,
                         const bool from_remote) {
  auto message = std::make_shared<messages::Message>();
  auto msg_header = message->mutable_header();
  auto msg_body = message->add_bodies();

  if (!error) {
    _current_id++;

    msg_header->set_connection_id(_current_id);
    auto connection =
        std::make_shared<tcp::Connection>(_current_id, _queue, socket, peer);
    _connections.insert(std::make_pair(_current_id, std::move(connection)));

    auto connection_ready = msg_body->mutable_connection_ready();

    connection_ready->set_from_remote(from_remote);

    _queue->publish(message);
    connection->read();
  } else {
    LOG_WARNING << "Could not create new connection to " << *peer << " due to "
                << error.message();

    auto connection_closed = msg_body->mutable_connection_closed();
    connection_closed->mutable_peer()->CopyFrom(*peer);
    _queue->publish(message);
  }
}

std::optional<Port> Tcp::connection_port(const Connection::ID id) const {
  auto got = _connections.find(id);
  if (got == _connections.end()) {
    return std::nullopt;
  }

  return std::make_optional(got->second->remote_port());
}

bool Tcp::terminate(const Connection::ID id) {
  auto got = _connections.find(id);
  if (got == _connections.end()) {
    return false;
  }
  got->second->terminate();

  return true;
}

bool Tcp::serialize(std::shared_ptr<messages::Message> message,
                    Buffer *header_tcp, Buffer *body_tcp) const {
  // TODO: use 1 output buffer
  LOG_DEBUG << this << " Before reinterpret and signing";
  auto header_pattern =
      reinterpret_cast<tcp::HeaderPattern *>(header_tcp->data());
  messages::to_buffer(*message, body_tcp);

  if (body_tcp->size() > MAX_MESSAGE_SIZE) {
    LOG_ERROR << "Message is too big (" << body_tcp->size() << ")";
    return false;
  }

  header_pattern->size = body_tcp->size();
  _keys->sign(body_tcp->data(), body_tcp->size(),
              reinterpret_cast<uint8_t *>(&header_pattern->signature));

  return true;
}

TransportLayer::SendResult Tcp::send(
    std::shared_ptr<messages::Message> message) const {
  if (_connections.size() == 0) {
    LOG_ERROR << "Could not send message because there is no connection";
    return SendResult::FAILED;
  }

  if (_stopped) {
    return SendResult::FAILED;
  }
  auto header_tcp =
      std::make_shared<Buffer>(sizeof(networking::tcp::HeaderPattern), 0);
  auto body_tcp = std::make_shared<Buffer>();

  std::cout << "\033[1;34mSending message[" << this->listening_port() << "]: >>"
            << *message << "<<\033[0m\n";
  if (!serialize(message, header_tcp.get(), body_tcp.get())) {
    return SendResult::FAILED;
  }

  int res_count = 0;
  for (auto &[_, connection] : _connections) {
    bool res_send = true;
    res_send &= connection->send(header_tcp);
    res_send &= connection->send(body_tcp);
    if (res_send) {
      res_count++;
    }
  }

  SendResult res;
  if (res_count == 0) {
    res = SendResult::FAILED;
  } else if (res_count == _connections.size()) {
    res = SendResult::FAILED;
  } else {
    res = SendResult::ALL_GOOD;
  }

  return res;
}

bool Tcp::send_unicast(std::shared_ptr<messages::Message> message) const {
  if (_stopped || !message->header().has_connection_id()) {
    return false;
  }

  auto got = _connections.find(message->header().connection_id());
  if (got == _connections.end()) {
    return false;
  }

  LOG_DEBUG << "\033[1;34mSending unicast [" << this->listening_port()
            << "]: >>" << *message << "<<\033[0m";

  auto header_tcp =
      std::make_shared<Buffer>(sizeof(networking::tcp::HeaderPattern), 0);
  auto body_tcp = std::make_shared<Buffer>();
  if (!serialize(message, header_tcp.get(), body_tcp.get())) {
    return false;
  }

  bool res = true;
  res &= got->second->send(header_tcp);
  res &= got->second->send(body_tcp);

  return res;
}

std::size_t Tcp::peer_count() const { return _connections.size(); }

void Tcp::stop() {
  if (!_stopped) {
    _stopped = true;
    _io_context.post([this]() { _acceptor.close(); });
    for (auto &[_, connection] : _connections) {
      connection->terminate();
    }
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
