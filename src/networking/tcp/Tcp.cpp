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
  auto handler = [this, socket, peer = std::move(peer)](
                     boost::system::error_code error,
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
    auto connection = std::make_shared<tcp::Connection>(_current_id, _queue,
                                                        socket, remote_peer);
    LOG_DEBUG << listening_port() << " new remote connection "
              << connection->ip() << ":"
              << connection->remote_port().value_or(0) << ":" << _current_id;
    _connections.insert(std::make_pair(_current_id, connection));
    LOG_DEBUG << pretty_connections();

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
    msg_header->mutable_key_pub()->CopyFrom(peer->key_pub());
    auto connection =
        std::make_shared<tcp::Connection>(_current_id, _queue, socket, peer);
    LOG_DEBUG << listening_port() << " new local connection "
              << connection->ip() << ":"
              << connection->remote_port().value_or(0) << ":" << _current_id;
    _connections.insert(std::make_pair(_current_id, connection));
    LOG_DEBUG << pretty_connections();

    auto connection_ready = msg_body->mutable_connection_ready();

    connection_ready->set_from_remote(false);

    _queue->publish(message);
    connection->read();
  } else {
    LOG_WARNING << "Could not create new connection to " << *peer << " : "
                << error.message();

    auto connection_closed = msg_body->mutable_connection_closed();
    peer->clear_connection_id();
    connection_closed->mutable_peer()->CopyFrom(*peer);
    _queue->publish(message);
  }
}

bool Tcp::terminate(const Connection::ID id) {
  std::unique_lock<std::mutex> lock_connection(_connections_mutex);
  auto got = _connections.find(id);
  if (got == _connections.end()) {
    LOG_ERROR << "terminate on connection not found " << id;
    return false;
  }
  got->second->remote_peer()->set_status(messages::Peer::UNREACHABLE);
  got->second->terminate();
  _connections.erase(got);
  return true;
}

std::shared_ptr<messages::Peer> Tcp::find_peer(const Connection::ID id) {
  auto connection = find(id);
  if (!connection) {
    return nullptr;
  }
  return connection->remote_peer();
}

bool Tcp::serialize(const messages::Message &message, Buffer *header_tcp,
                    Buffer *body_tcp) const {
  // TODO: use 1 output buffer
  LOG_DEBUG << this << " Before reinterpret and signing";
  auto header_pattern =
      reinterpret_cast<tcp::HeaderPattern *>(header_tcp->data());
  messages::to_buffer(message, body_tcp);

  if (body_tcp->size() > MAX_MESSAGE_SIZE) {
    LOG_ERROR << "Message is too big (" << body_tcp->size() << ")";
    return false;
  }

  header_pattern->size = body_tcp->size();
  _keys->sign(body_tcp->data(), body_tcp->size(),
              reinterpret_cast<uint8_t *>(&header_pattern->signature));

  return true;
}

/**
 * send a message to a specific peer
 * \param message a message to send
 * \param id connection id of the peer to send to
 * \return failure or all_good
 */
TransportLayer::SendResult Tcp::send(const messages::Message &message,
                                     const Connection::ID id) const {
  const auto connection = find(id);
  if (!connection) {
    LOG_WARNING << "not sending message because could not find connection";
    return SendResult::FAILED;
  }
  const auto port_opt = connection->remote_port();
  if (!port_opt) {
    return SendResult::FAILED;
  }
  LOG_DEBUG << "Sending unicast [" << this->listening_port() << " -> "
            << *port_opt << "]: >>" << message;

  auto header_tcp =
      std::make_shared<Buffer>(sizeof(networking::tcp::HeaderPattern), 0);
  auto body_tcp = std::make_shared<Buffer>();
  if (!serialize(message, header_tcp.get(), body_tcp.get())) {
    return SendResult::FAILED;
  }
  if (connection->send(header_tcp) && connection->send(body_tcp)) {
    return SendResult::ALL_GOOD;
  } else {
    return SendResult::FAILED;
  }
}

TransportLayer::SendResult Tcp::send_one(
    const messages::Message &message) const {
  auto peer_it = _peers->begin(messages::Peer::CONNECTED);
  if (peer_it != _peers->end()) {
    return send(message, peer_it->connection_id());
  }
  return SendResult::FAILED;
}

TransportLayer::SendResult Tcp::send_all(
    const messages::Message &message) const {
  const auto connected_peers = _peers->connected_peers();
  bool one_good = false;
  bool one_failed = false;
  for (const auto peer : connected_peers) {
    if (send(message, peer->connection_id()) == SendResult::ALL_GOOD) {
      one_good = true;
    } else {
      one_failed = true;
    }
  }
  if (!one_good) {
    return SendResult::FAILED;
  }
  if (one_failed) {
    return SendResult::ONE_OR_MORE_SENT;
  }
  return SendResult::ALL_GOOD;
}

std::shared_ptr<tcp::Connection> Tcp::find(const Connection::ID id) const {
  std::unique_lock<std::mutex> lock_connection(_connections_mutex);
  auto got = _connections.find(id);
  if (got == _connections.end()) {
    return nullptr;
  }

  return got->second;
}

bool Tcp::reply(std::shared_ptr<messages::Message> message) const {
  if (_stopped || !message->header().has_connection_id()) {
    LOG_WARNING << "not sending message " << _stopped;
    return false;
  }
  return send(*message, message->header().connection_id()) ==
         SendResult::ALL_GOOD;
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

std::vector<std::shared_ptr<messages::Peer>> Tcp::remote_peers() const {
  std::unique_lock<std::mutex> lock_connection(_connections_mutex);
  std::vector<std::shared_ptr<messages::Peer>> remote_peers;
  for (const auto &[_, connection] : _connections) {
    remote_peers.push_back(connection->remote_peer());
  }
  return remote_peers;
}

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

std::string Tcp::pretty_connections() {
  std::stringstream result;
  result << listening_port() << " pretty connections";
  for (const auto &[id, connection] : _connections) {
    auto peer = connection->remote_peer();
    result << " " << id << ":" << peer->endpoint() << ":" << peer->port() << ":"
           << _Peer_Status_Name(peer->status()) << ":" << peer->connection_id();
  }
  return result.str();
}

Tcp::~Tcp() {
  stop();
  LOG_DEBUG << this << " TCP killed";
}

}  // namespace networking
}  // namespace neuro
