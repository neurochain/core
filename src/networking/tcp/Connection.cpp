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
                       const std::shared_ptr<tcp::socket> &socket,
                       const messages::config::Networking &config)
    : ::neuro::networking::Connection::Connection(id, queue),
      _header(sizeof(HeaderPattern), 0),
      _buffer(128, 0),
      _socket(socket),
      _remote_peer(config){
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
          LOG_WARNING << "read header error " << error.message();
          _this->terminate();
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
          LOG_WARNING << "read body error " << error.message();
          _this->terminate();
          return;
        }
        const auto header_pattern =
            reinterpret_cast<HeaderPattern *>(_this->_header.data());

        auto message = std::make_shared<messages::Message>();
        messages::from_buffer(_this->_buffer, message.get());
        LOG_DEBUG << "Receiving [" << _socket->local_endpoint().port() << " <- "
                  << _remote_peer.port() << "]: <<" << *message;
        auto header = message->mutable_header();

        header->set_connection_id(_id);
        auto signature = header->mutable_signature();
        signature->set_type(messages::Hash::SHA256);
        signature->set_data(header_pattern->signature,
                            sizeof(header_pattern->signature));

        // validate the MessageVersion from the header
        if (header->version() != neuro::MessageVersion) {
          LOG_INFO << "Killing connection because received message from wrong version (" << header->version() << ")";
          _this->terminate();
          return;
        }
        for (auto &body : *message->mutable_bodies()) {
          const auto type = get_type(body);
          if (type == messages::Type::kHello) {
	    auto* hello = body.mutable_hello();
	    hello->mutable_peer()->set_endpoint(_socket->remote_endpoint().address().to_string());
            _remote_peer.CopyFrom(hello->peer());
	    LOG_TRACE  << "remote ip> " <<  _socket->remote_endpoint().address().to_string() << std::endl;
          }
        }

        if (!_this->_remote_peer.has_key_pub()) {
          LOG_INFO << "Killing connection because received message without key pub";
          _this->terminate();
          return;
        }
        const auto key_pub = _this->_remote_peer.key_pub();

        crypto::KeyPub ecc_pub(key_pub);

        const auto check =
            ecc_pub.verify(_this->_buffer, header_pattern->signature,
                           sizeof(header_pattern->signature));

        if (!check) {
          LOG_INFO
            << "Killing connection because received message with wrong signature"
            << _this->_queue << " " << messages::to_json(*message) << " "
            << _remote_peer << std::endl;
          _this->terminate();
          return;
        }
        message->mutable_header()->mutable_key_pub()->CopyFrom(
            _remote_peer.key_pub());
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
          LOG_WARNING << "send error " << error.message();
          _this->terminate();
          return false;
        }
        return true;
      });
  return true;
}

void Connection::close() { _socket->close(); }

void Connection::terminate() {
  LOG_INFO << this << " " << _id << " Killing connection";
  boost::system::error_code ec;
  _socket->shutdown(tcp::socket::shutdown_both, ec);
  if (ec) {
    LOG_DEBUG << "can't shutdown connection socket to : "
              << remote_port().value_or(0) << " : " << ec.message();
  }
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
  close();
}
}  // namespace tcp
}  // namespace networking
}  // namespace neuro
