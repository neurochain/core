#include "networking/tcp/Connection.hpp"
#include "common/logger.hpp"
#include "common/types.hpp"
#include "messages/Queue.hpp"
#include "networking/tcp/Tcp.hpp"

namespace neuro {
namespace networking {
namespace tcp {

std::shared_ptr<tcp::socket> Connection::socket() { return _socket; }

void Connection::read() { read_header(); }

void Connection::read_header() {
  boost::asio::async_read(
      *_socket, boost::asio::buffer(_header.data(), _header.size()),
      [_this=ptr()](const boost::system::error_code &error, std::size_t bytes_read) {
        if (error) {
          LOG_ERROR << _this << " " << __LINE__ << " Killing connection "
                    << error;
          _this->terminate();
          return;
        }

        auto header_pattern =
	  reinterpret_cast<HeaderPattern *>(_this->_header.data());
        _this->_buffer.resize(header_pattern->size);
        _this->read_body();
      });
}

void Connection::read_body() {
  boost::asio::async_read(
      *_socket, boost::asio::buffer(_buffer.data(), _buffer.size()),
      [_this=ptr()](const boost::system::error_code &error, std::size_t bytes_read) {
        if (error) {
          LOG_ERROR << _this << " " << __LINE__ << " Killing connection "
                    << error;
          _this->terminate();
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
          _this->terminate();
          return;
        }

        for (const auto &body : message->bodies()) {
          const auto type = get_type(body);
          LOG_DEBUG << _this << " read_body TYPE " << type;
          if (type == messages::Type::kHello) {
            auto hello = body.hello();

            if (hello.has_listen_port()) {
              _this->_listen_port = hello.listen_port();
            }

            if (!_this->_remote_peer->has_key_pub()) {
              LOG_INFO << "Updating peer with hello key pub";
              _this->_remote_peer->mutable_key_pub()->CopyFrom(hello.key_pub());
            }
          }
        }
        std::cout << "\033[1;32mMessage received: " << *message << "\033[0m"
                  << std::endl;

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

        const auto check = ecc_pub.verify(_this->_buffer, header_pattern->signature,
                                          sizeof(header_pattern->signature));

        if (!check) {
          LOG_ERROR << "Bad signature, dropping message";
          _this->read_header();
          return;
        }

        _this->_queue->publish(message);
        _this->read_header();
      });
}

bool Connection::send(const Buffer &message) {
  boost::asio::async_write(*_socket,
                           boost::asio::buffer(message.data(), message.size()),
                           [_this=ptr()](const boost::system::error_code &error,
                                  std::size_t bytes_transferred) {
                             if (error) {
                               LOG_ERROR << "Could not send message";
                               LOG_ERROR << _this << " " << __LINE__
                                         << " Killing connection " << error;
                               _this->terminate();
                               return false;
                             }
                             return true;
                           });
  return true;
}

void Connection::terminate() {
  std::lock_guard<std::mutex> m(_connection_mutex);
  if (_is_dead) {
    return;
  }
  _socket->close();
  auto message = std::make_shared<messages::Message>();
  auto header = message->mutable_header();
  auto peer = header->mutable_peer();
  peer->CopyFrom(*_remote_peer);
  auto body = message->add_bodies();
  body->mutable_connection_closed();
  _queue->publish(message);
  _is_dead = true;
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

std::shared_ptr<messages::Peer> Connection::remote_peer() {
  return _remote_peer;
}
Connection::~Connection() {
  _socket->close();

  while (!_is_dead) {
    std::this_thread::yield();
  }
  LOG_DEBUG << this << " Connection killed";
}
}  // namespace tcp
}  // namespace networking
}  // namespace neuro
