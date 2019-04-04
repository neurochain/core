#include <cassert>

#include "common/logger.hpp"
#include "common/types.hpp"
#include "messages/Queue.hpp"
#include "networking/tcp/Connection.hpp"
#include "networking/tcp/Tcp.hpp"

namespace neuro {
namespace networking {
namespace tcp {

Connection::Connection(const ID id, messages::Queue *queue,
                       const std::shared_ptr<tcp::socket> &socket,
                       const messages::Peer &remote_peer)
    : ::neuro::networking::Connection::Connection(id, queue),
      _header(sizeof(HeaderPattern), 0),
      _buffer(128, 0),
      _socket(socket),
      _remote_peer(remote_peer) {
  assert(_socket != nullptr);
}

Connection::Connection(const ID id, messages::Queue *queue,
                       const std::shared_ptr<tcp::socket> &socket)
    : ::neuro::networking::Connection::Connection(id, queue),
      _header(sizeof(HeaderPattern), 0),
      _buffer(128, 0),
      _socket(socket) {
  assert(_socket != nullptr);
}

std::shared_ptr<const tcp::socket> Connection::socket() const {
  return _socket;
}

void Connection::read() { read_header(); }

void Connection::read_header() {
  boost::asio::async_read(
      *_socket, boost::asio::buffer(_header.data(), _header.size()),
      [_this = ptr()](const boost::system::error_code &error,
                      std::size_t bytes_read) {
        if (error) {
          if (error == boost::asio::error::eof) {
            _this->terminate();
          } else {
            LOG_ERROR << _this << " " << __LINE__ << " Killing connection "
                      << error;
          }
          return;
        }
        const auto header_pattern =
            reinterpret_cast<HeaderPattern *>(_this->_header.data());
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
          if (error == boost::asio::error::eof) {
            _this->terminate();
          }
          return;
        }
        const auto header_pattern =
            reinterpret_cast<HeaderPattern *>(_this->_header.data());

        auto message = std::make_shared<messages::Message>();
        messages::from_buffer(_this->_buffer, message.get());

        auto header = message->mutable_header();

        header->set_connection_id(_id);
        auto signature = header->mutable_signature();
        signature->set_type(messages::Hash::SHA256);
        signature->set_data(header_pattern->signature,
                            sizeof(header_pattern->signature));

        // validate the MessageVersion from the header
        if (header->version() != neuro::MessageVersion) {
          return;
        }
        for (const auto &body : message->bodies()) {
          const auto type = get_type(body);
          if (type == messages::Type::kHello) {
            const auto hello = body.hello();
            _remote_peer.CopyFrom(hello.peer());
          }
        }

        if (!_this->_remote_peer.has_key_pub()) {
          _this->read_header();
          return;
        }
        const auto key_pub = _this->_remote_peer.key_pub();

        crypto::EccPub ecc_pub(key_pub);

        const auto check =
            ecc_pub.verify(_this->_buffer, header_pattern->signature,
                           sizeof(header_pattern->signature));

        if (!check) {
          return;
        }
        message->mutable_header()->mutable_key_pub()->CopyFrom(
            _remote_peer.key_pub());
        std::cout << "received " << _this->_queue << " "
                  <<  messages::to_json(*message) << " " << _remote_peer << std::endl;
        _this->_queue->publish(message);
        _this->read_header();
      });
}

bool Connection::send(std::shared_ptr<Buffer> &message) {
  boost::asio::async_write(
      *_socket, boost::asio::buffer(message->data(), message->size()),
      [_this = ptr(), message](const boost::system::error_code &error,
                               std::size_t bytes_transferred) {
        if (error) {
          _this->terminate();
          return false;
        }
        return true;
      });
  return true;
}

void Connection::close() { _socket->close(); }

void Connection::terminate() {
  _socket->close();
  auto message = std::make_shared<messages::Message>();
  auto header = message->mutable_header();
  header->set_connection_id(_id);
  auto body = message->add_bodies();
  body->mutable_connection_closed();

  _queue->publish(message);
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

const messages::Peer Connection::remote_peer() const { return _remote_peer; }
Connection::~Connection() {
  terminate();
}
}  // namespace tcp
}  // namespace networking
}  // namespace neuro
