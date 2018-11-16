#include "networking/tcp/Connection.hpp"
#include "common/check.hpp"
#include "common/logger.hpp"
#include "common/types.hpp"
#include "messages/Queue.hpp"
#include "networking/tcp/Tcp.hpp"

namespace neuro {
namespace networking {
namespace tcp {

Connection::Connection(const ID id,
                       networking::TransportLayer::ID transport_layer_id,
                       std::shared_ptr<boost::asio::io_service> io_context,
                       std::shared_ptr<messages::Queue> queue,
                       std::shared_ptr<tcp::socket> socket,
                       std::shared_ptr<messages::Peer> remote_peer)
    : ::neuro::networking::Connection::Connection(id, transport_layer_id,
                                                  queue),
      _io_context(io_context),
      _header(sizeof(HeaderPattern), 0),
      _buffer(128, 0),
      _socket(socket),
      _remote_peer(remote_peer) {
  CHECK(_socket != nullptr, "_socket is nullptr");
  CHECK(_remote_peer != nullptr, "_remote_peer is nullptr");
  _listen_port = _remote_peer->port();
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
          LOG_ERROR << _this << " " << __LINE__ << " Killing connection "
                    << error;
          return;
        }
        auto header_pattern =
            reinterpret_cast<HeaderPattern *>(_this->_header.data());
        _this->read_body(header_pattern->size);
      });
}

void Connection::read_body(std::size_t body_size) {
  _buffer.resize(body_size);
  boost::asio::async_read(
      *_socket, boost::asio::buffer(_buffer.data(), _buffer.size()),
      [_this = ptr()](const boost::system::error_code &error,
                      std::size_t bytes_read) {
        if (error) {
          LOG_ERROR << _this << " " << __LINE__ << " Killing connection "
                    << error;
          return;
        }
        const auto header_pattern =
            reinterpret_cast<HeaderPattern *>(_this->_header.data());

        auto message = std::make_shared<messages::Message>();
        messages::from_buffer(_this->_buffer, message.get());

        auto header = message->mutable_header();

        header->mutable_peer()->CopyFrom(*_this->_remote_peer);
        auto signature = header->mutable_signature();
        signature->set_type(messages::Hash::SHA256);
        signature->set_data(header_pattern->signature,
                            sizeof(header_pattern->signature));

        // validate the MessageVersion from the header
        if (header->version() != neuro::MessageVersion) {
          LOG_ERROR << " MessageVersion not corresponding: "
                    << neuro::MessageVersion << " (mine) vs "
                    << header->version() << " )";
          return;
        }
        for (const auto &body : message->bodies()) {
          const auto type = get_type(body);
          LOG_DEBUG << _this << " read_body TYPE " << type;
          if (type == messages::Type::kHello) {
            const auto hello = body.hello();

            if (hello.has_listen_port()) {
              _this->_listen_port = hello.listen_port();
            }

            if (!_this->_remote_peer->has_key_pub()) {
              LOG_INFO << "Updating peer with hello key pub";
              _this->_remote_peer->mutable_key_pub()->CopyFrom(hello.key_pub());
            }
          }
        }
        LOG_DEBUG << "\033[1;32mMessage received: " << *message << "\033[0m";

        if (!_this->_remote_peer->has_key_pub()) {
          LOG_ERROR << "Not Key pub set";
          _this->read_header();
          return;
        }
        crypto::EccPub ecc_pub;
        const auto key_pub = _this->_remote_peer->key_pub();

        ecc_pub.load(
            reinterpret_cast<const uint8_t *>(key_pub.raw_data().data()),
            key_pub.raw_data().size());

        const auto check =
            ecc_pub.verify(_this->_buffer, header_pattern->signature,
                           sizeof(header_pattern->signature));

        if (!check) {
          LOG_ERROR << "Bad signature, dropping message";
        } else {
          _this->_queue->publish(message);
        }
        _this->read_header();
      });
}

bool Connection::send(const Buffer &message) {
  boost::asio::async_write(
      *_socket, boost::asio::buffer(message.data(), message.size()),
      [_this = ptr()](const boost::system::error_code &error,
                      std::size_t bytes_transferred) {
        if (error) {
          LOG_ERROR << "Could not send message";
          LOG_ERROR << _this << " " << __LINE__ << " Killing connection "
                    << error;
          _this->close();
          return false;
        }
        return true;
      });
  return true;
}

void Connection::close() { _socket->close(); }

void Connection::terminate() {
  close();
  auto message = std::make_shared<messages::Message>();
  auto header = message->mutable_header();
  auto peer = header->mutable_peer();
  peer->CopyFrom(*_remote_peer);
  auto body = message->add_bodies();
  body->mutable_connection_closed();
  _queue->publish(message);
}

const IP Connection::remote_ip() const {
  const auto endpoint = _socket->remote_endpoint();
  return endpoint.address();
}

const Port Connection::remote_port() const {
  const auto endpoint = _socket->remote_endpoint();
  return static_cast<Port>(endpoint.port());
}

const Port Connection::listen_port() const { return _listen_port; }

std::shared_ptr<const messages::Peer> Connection::remote_peer() const {
  return _remote_peer;
}
Connection::~Connection() {
  terminate();
  LOG_DEBUG << this << " Connection killed";
}
}  // namespace tcp
}  // namespace networking
}  // namespace neuro
