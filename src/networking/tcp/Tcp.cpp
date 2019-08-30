#include <assert.h>
#include <boost/asio/impl/io_context.ipp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/system/error_code.hpp>

#include "common/logger.hpp"
#include "common/types.hpp"
#include "config.pb.h"
#include "crypto/Ecc.hpp"
#include "messages/Peer.hpp"
#include "messages/Peers.hpp"
#include "messages/Queue.hpp"
#include "networking/TransportLayer.hpp"
#include "networking/tcp/Tcp.hpp"

namespace neuro {
namespace networking {
using namespace std::chrono_literals;

Tcp::Tcp(messages::Queue *queue, messages::Peers *peers, crypto::Ecc *keys,
         const messages::config::Networking &config)
    : TransportLayer(queue, peers, keys),
      _stopped(false),
      _listening_port(config.tcp().port()),
      _io_context(),
      _resolver(_io_context),
      _acceptor(_io_context,
                bai::tcp::endpoint(bai::tcp::v4(), _listening_port)),
      _config(config) {
  assert(peers);

  while (!_acceptor.is_open()) {
    std::this_thread::yield();
    LOG_DEBUG << "Waiting for acceptor to be open";
  }
  start_accept();
  _io_context_thread = std::thread([this]() {
    boost::system::error_code ec;
    this->_io_context.run(ec);
    LOG_INFO << __FILE__ << ":" << __LINE__ << "> io_context exited " << ec;
  });
}

void Tcp::start_accept() {
  if (!_stopped) {
    _new_socket = std::make_shared<bai::tcp::socket>(_io_context);
    _acceptor.async_accept(
        *_new_socket.get(),
        boost::bind(&Tcp::accept, this, boost::asio::placeholders::error));
  }
}

void Tcp::accept(const boost::system::error_code &error) {
  if (!error) {
    const auto remote_endpoint =
        _new_socket->remote_endpoint().address().to_string();
    this->new_connection_from_remote(_new_socket, error);
  } else {
    LOG_WARNING << "Rejected connection " << error.message();
  }
  _new_socket.reset();  // TODO why?
  start_accept();
}

bool Tcp::connect(std::shared_ptr<messages::Peer> peer) {
  if (_stopped) {
    return false;
  }

  bai::tcp::resolver resolver(_io_context);
  bai::tcp::resolver::query query(peer->endpoint(),
                                  std::to_string(peer->port()));
  bai::tcp::resolver::iterator endpoint_iterator = resolver.resolve(query);

  auto socket = std::make_shared<bai::tcp::socket>(_io_context);
  auto handler = [this, socket, peer=std::move(peer)](boost::system::error_code error,
                                      bai::tcp::resolver::iterator iterator) {
    this->new_connection_local(socket, error, peer);
  };
  boost::asio::async_connect(*socket, endpoint_iterator, handler);
  return true;
}

Port Tcp::listening_port() const { return _listening_port; }

void Tcp::new_connection_from_remote(std::shared_ptr<bai::tcp::socket> socket,
                                     const boost::system::error_code &error) {
  std::unique_lock<std::mutex> lock_connection(_connections_mutex);
  auto message = std::make_shared<messages::Message>();
  auto msg_header = message->mutable_header();
  auto msg_body = message->add_bodies();

  if (!error) {
    ++_current_id;

    msg_header->set_connection_id(_current_id);
    auto remote_peer = std::make_shared<messages::Peer>(_config);
    auto connection =
        std::make_shared<tcp::Connection>(_current_id, _queue, socket, remote_peer);
    _connections.insert(std::make_pair(_current_id, connection));

    auto connection_ready = msg_body->mutable_connection_ready();

    connection_ready->set_from_remote(true);

    _queue->publish(message);
    connection->read();
  } else {
    LOG_WARNING << "Could not create new connection : " << error.message();

    msg_body->mutable_connection_closed();
    _queue->publish(message);
  }
}

void Tcp::new_connection_local(std::shared_ptr<bai::tcp::socket> socket,
                               const boost::system::error_code &error,
                               std::shared_ptr<messages::Peer> peer) {
  std::unique_lock<std::mutex> lock_connection(_connections_mutex);
  auto message = std::make_shared<messages::Message>();
  auto msg_header = message->mutable_header();
  auto msg_body = message->add_bodies();

  if (!error) {
    ++_current_id;

    msg_header->set_connection_id(_current_id);
    auto connection =
        std::make_shared<tcp::Connection>(_current_id, _queue, socket, peer);
    _connections.insert(std::make_pair(_current_id, connection));

    auto connection_ready = msg_body->mutable_connection_ready();

    connection_ready->set_from_remote(false);

    _queue->publish(message);
    connection->read();
  } else {
    LOG_WARNING << "Could not create new connection to " << *peer << " : "
                << error.message();

    auto connection_closed = msg_body->mutable_connection_closed();
    connection_closed->mutable_peer()->CopyFrom(*peer);
    _queue->publish(message);
  }
}

bool Tcp::terminate(const Connection::ID id) {
  std::unique_lock<std::mutex> lock_connection(_connections_mutex);
  auto got = _connections.find(id);
  if (got == _connections.end()) {
    LOG_TRACE << "trax> could not terminating " << id << " " << _connections.size();
    return false;
  }
  got->second->terminate();
  _connections.erase(got);
  return true;
}

std::optional<std::shared_ptr<messages::Peer> > Tcp::find_peer(const Connection::ID id) {
  auto connection = find(id);
  if (!connection) {
    return std::nullopt;
  }
  return (*connection)->remote_peer();
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
  std::unique_lock<std::mutex> lock_connection(_connections_mutex);
  if (_connections.size() == 0) {
    LOG_ERROR << "Could not send message because there is no connection "
              << message;
    return SendResult::FAILED;
  }

  if (_stopped) {
    return SendResult::FAILED;
  }
  auto header_tcp =
      std::make_shared<Buffer>(sizeof(networking::tcp::HeaderPattern), 0);
  auto body_tcp = std::make_shared<Buffer>();

  if (!serialize(message, header_tcp.get(), body_tcp.get())) {
    LOG_WARNING << "Could not serialize message";
    return SendResult::FAILED;
  }

  LOG_DEBUG << "Sending message[" << this->listening_port() << "]: >>"
            << *message << " " << _connections.size();

  uint16_t res_count = 0;
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
  } else if (res_count != _connections.size()) {
    res = SendResult::FAILED;
  } else {
    res = SendResult::ALL_GOOD;
  }

  return res;
}

std::optional<tcp::Connection *> Tcp::find(const Connection::ID id) const {
  std::unique_lock<std::mutex> lock_connection(_connections_mutex);
  auto got = _connections.find(id);
  if (got == _connections.end()) {
    return std::nullopt;
  }

  return got->second.get();
}

bool Tcp::send_unicast(std::shared_ptr<messages::Message> message) const {
  if (_stopped || !message->header().has_connection_id()) {
    LOG_WARNING << "not sending message " << _stopped;
    return false;
  }

  const auto connection_opt = find(message->header().connection_id());
  if (!connection_opt) {
    LOG_WARNING << "not sending message because could not find connection";
    return false;
  }
  const auto connection = *connection_opt;
  const auto port_opt = connection->remote_port();
  if (!port_opt) {
    return false;
  }
  LOG_DEBUG << "Sending unicast [" << this->listening_port() << " -> "
            << *port_opt << "]: >>" << *message;

  auto header_tcp =
      std::make_shared<Buffer>(sizeof(networking::tcp::HeaderPattern), 0);
  auto body_tcp = std::make_shared<Buffer>();
  if (!serialize(message, header_tcp.get(), body_tcp.get())) {
    return false;
  }
  bool res = true;
  res &= connection->send(header_tcp);
  res &= connection->send(body_tcp);

  return res;
}

void Tcp::clean_old_connections(int delta) {
  std::unique_lock<std::mutex> lock_connection(_connections_mutex);
  const auto current_time = ::neuro::time() - delta;
  for (auto &[_, connection] : _connections) {
    auto remote_peer = connection->remote_peer();
    if ((connection->init_ts() < current_time) &&
        (remote_peer->status() != messages::Peer::CONNECTED)) {
      connection->terminate();
    }
  }
}

/**
 * count the number of active TCP connexion
 * (either accepted one or attempting one)
 * \return the number of active connexion
 */
std::size_t Tcp::peer_count() const { return _connections.size(); }

void Tcp::stop() {
  std::unique_lock<std::mutex> lock_connection(_connections_mutex);
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
