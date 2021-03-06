#include <cassert>

#include "common/logger.hpp"
#include "config.pb.h"
#include "messages/Peer.hpp"
#include "messages/Queue.hpp"
#include "networking/Connection.hpp"
#include "networking/tcp/Connection.hpp"

namespace neuro {
namespace networking {
namespace tcp {

Connection::Connection(const ID id, messages::Queue *queue,
                       const std::shared_ptr<tcp::socket> &socket,
                       std::shared_ptr<messages::Peer> remote_peer)
    : ::neuro::networking::Connection::Connection(id, queue),
      _header(sizeof(HeaderPattern), 0),
      _buffer(128, 0),
      _socket(socket),
      _remote_peer(remote_peer) {
  assert(_socket != nullptr);
  remote_peer->set_connection_id(id);
}

std::shared_ptr<const tcp::socket> Connection::socket() const {
  return _socket;
}

void Connection::read() { read_header(); }

const std::string Connection::ip() const {
  auto ip = remote_ip();
  if (ip) {
    return ip->to_string();
  } else {
    return "(no ip)";
  }
}

void Connection::read_header() {
  boost::asio::async_read(
      *_socket, boost::asio::buffer(_header.data(), _header.size()),
      [_this = ptr()](const boost::system::error_code &error,
                      std::size_t bytes_read) {
        if (error) {
          LOG_WARNING << " read header error " << error.message() << " "
                      << _this->ip() << ":" << *_this->remote_port() << ":"
                      << _this->id();
          _this->terminate();
          return;
        }
        const auto header_pattern =
            reinterpret_cast<const HeaderPattern *>(_this->_header.data());
        if (header_pattern->size > MAX_MESSAGE_SIZE) {
          LOG_WARNING << "Receiving message too big " << header_pattern->size;
          return;
        }
        _this->read_body(header_pattern->size);
      });
}

void Connection::read_body(std::size_t body_size) {
  _buffer.resize(body_size);
  boost::asio::async_read(
      *_socket, boost::asio::buffer(_buffer.data(), _buffer.size()),
      [_this = ptr(), this](const boost::system::error_code &error,
                            std::size_t bytes_read) {
        if (error) {
          LOG_WARNING << this << " read body error " << error.message() << " "
                      << ip();
          _this->terminate();
          return;
        }

        const auto header_pattern =
            reinterpret_cast<HeaderPattern *>(_this->_header.data());

        auto message = std::make_shared<messages::Message>();
        messages::from_buffer(_this->_buffer, message.get());
        auto header = message->mutable_header();

        header->set_connection_id(_id);
        auto signature = header->mutable_signature();
        signature->set_data(
            reinterpret_cast<const char *>(header_pattern->signature),
            sizeof(header_pattern->signature));

        // validate the MessageVersion from the header
        if (header->version() != neuro::MessageVersion) {
          LOG_INFO << "Killing connection because received message from wrong "
                      "version ("
                   << header->version() << ") " << ip() << ":"
                   << remote_port().value_or(0);
          _this->terminate();
          return;
        }
        for (auto &body : *message->mutable_bodies()) {
          const auto type = get_type(body);
          if (type == messages::Type::kHello) {
            boost::system::error_code ec;
            const auto endpoint = _socket->remote_endpoint(ec);
            if (ec) {
              LOG_DEBUG << "got an hello message from disconnected endpoint "
                        << ec.message() << std::endl;
              _this->terminate();
              return;
            } else {
              auto *hello = body.mutable_hello();
              hello->mutable_peer()->set_endpoint(
                  endpoint.address().to_string());
              _remote_peer->CopyFrom(hello->peer());
              _remote_peer->set_connection_id(_id);
            }
          }
        }

        if (!_this->_remote_peer->has_key_pub()) {
          LOG_INFO
              << "Killing connection because received message without key pub "
              << ip() << ":" << remote_port().value_or(0) << ":" << _id;
          _this->terminate();
          return;
        }
        const auto key_pub = _this->_remote_peer->key_pub();

        crypto::KeyPub ecc_pub(key_pub);

        const auto check =
            ecc_pub.verify(_this->_buffer, header_pattern->signature,
                           sizeof(header_pattern->signature));

        if (!check) {
          LOG_INFO << "Bad signature on incomming message";
          _this->terminate();
          return;
        }
        try {
          LOG_DEBUG << "Receiving [" << _socket->remote_endpoint() << ":"
                    << _remote_peer->port() << "]: " << *message;
        } catch (...) {
          _this->_buffer.save("conf/crashed.proto");
          _this->terminate();
          return;
        }
        message->mutable_header()->mutable_key_pub()->CopyFrom(
            _remote_peer->key_pub());

        bool is_hello = false;
        bool is_world = false;
        if (message->bodies_size() > 0) {
          const auto type = get_type(message->bodies(0));
          if (type == messages::Type::kHello) {
            is_hello = message->bodies_size() == 1;
          } else if (type == messages::Type::kWorld) {
            is_world = true;
          }
        }
        if (_remote_peer->status() == messages::Peer::CONNECTED || is_hello ||
            (_remote_peer->status() == messages::Peer::CONNECTING &&
             is_world)) {
          _this->_queue->push(message);
        } else {
          LOG_WARNING << "Message from " << _remote_peer
                      << " was not sent to the queue because the sender "
                         "is not a connected peer "
                      << *message;
          _this->terminate();
          return;
        }
        _this->read_header();
      });
}

bool Connection::send(std::shared_ptr<Buffer> &message) {
  boost::asio::async_write(
      *_socket, boost::asio::buffer(message->data(), message->size()),
      [_this = ptr(), message](const boost::system::error_code &error,
                               std::size_t bytes_transferred) {
        if (error) {
          LOG_WARNING << "send error " << error.message();
          _this->terminate();
          return false;
        }
        return true;
      });
  return true;
}

void Connection::close() { _socket->close(); }

void Connection::terminate(bool from_inside) const {
  LOG_INFO << this << " " << _id << " Killing connection " << ip() << ":"
           << remote_port().value_or(0);
  boost::system::error_code ec;
  _socket->shutdown(tcp::socket::shutdown_both, ec);
  if (ec) {
    LOG_ERROR << "can't shutdown connection socket to: id " << _id << " "
              << remote_port().value_or(0) << " : " << ec.message();
  }
  if (!from_inside) {
    // if terminate comes from outside, we don't need to
    // send a message to the bot.
    return;
  }
  auto message = std::make_shared<messages::Message>();
  auto header = message->mutable_header();
  header->set_connection_id(_id);
  auto body = message->add_bodies();
  body->mutable_connection_closed();
  _remote_peer->set_status(messages::Peer::UNREACHABLE);
  _remote_peer->clear_connection_id();
  _queue->push(message);
}

const std::optional<IP> Connection::remote_ip() const {
  boost::system::error_code ec;
  const auto endpoint = _socket->remote_endpoint(ec);
  if (ec) {
    return {};
  }
  return endpoint.address();
}

const std::optional<Port> Connection::remote_port() const {
  boost::system::error_code ec;
  const auto endpoint = _socket->remote_endpoint(ec);
  if (ec) {
    return {};
  }
  return static_cast<Port>(endpoint.port());
}

const std::optional<Port> Connection::local_port() const {
  boost::system::error_code ec;
  const auto endpoint = _socket->local_endpoint(ec);
  if (ec) {
    return {};
  }
  return static_cast<Port>(endpoint.port());
}

std::shared_ptr<messages::Peer> Connection::remote_peer() const {
  return _remote_peer;
}

std::shared_ptr<Connection> Connection::ptr() { return shared_from_this(); }

Connection::~Connection() {
  LOG_DEBUG << local_port().value_or(0) << " closing connection "
            << remote_port().value_or(0) << ":" << _id;
  close();
}
}  // namespace tcp
}  // namespace networking
}  // namespace neuro
