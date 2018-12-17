#include <cassert>

#include "common/logger.hpp"
#include "common/types.hpp"
#include "crypto/Ecc.hpp"
#include "messages/Queue.hpp"
#include "networking/tcp/Connection.hpp"
#include "networking/tcp/HeaderPattern.hpp"
#include "networking/tcp/Tcp.hpp"

namespace neuro {
namespace networking {
namespace tcp {

bool Connection::Packet::serialize(const crypto::Ecc &keys,
                                   const messages::Message &message,
                                   Buffer &header_tcp, Buffer &body_tcp) {
  LOG_DEBUG << " Before reinterpret and signing";
  auto header_pattern = reinterpret_cast<HeaderPattern *>(header_tcp.data());
  messages::to_buffer(message, &body_tcp);

  if (body_tcp.size() > MAX_MESSAGE_SIZE) {
    LOG_ERROR << "Message is too big (" << body_tcp.size() << ")";
    return false;
  }

  header_pattern->size = body_tcp.size();
  keys.sign(body_tcp.data(), body_tcp.size(),
            reinterpret_cast<uint8_t *>(&header_pattern->signature));
  return true;
}

std::shared_ptr<messages::Message> Connection::Packet::deserialize(
    const std::optional<crypto::EccPub> &pub_key, const Buffer &header_tcp,
    const Buffer &body_tcp) {
  const auto header_pattern =
      reinterpret_cast<const HeaderPattern *>(header_tcp.data());

  auto message = std::make_shared<messages::Message>();
  if (!messages::from_buffer(body_tcp, message.get())) {
    LOG_ERROR << "Failed to parse message.";
    message.reset();
    return message;
  }

  auto header = message->mutable_header();

  // validate the MessageVersion from the header
  if (header->version() != neuro::MessageVersion) {
    LOG_ERROR << " MessageVersion not corresponding: " << neuro::MessageVersion
              << " (mine) vs " << header->version() << " )";
    message.reset();
    return message;
  }

  auto signature = header->mutable_signature();
  signature->set_type(messages::Hash::SHA256);
  signature->set_data(header_pattern->signature,
                      sizeof(header_pattern->signature));

  if (!!pub_key &&
      pub_key->verify(body_tcp, header_pattern->signature,
                      sizeof(header_pattern->signature)) == false) {
    LOG_ERROR << "Bad signature, dropping message";
    message.reset();
    return message;
  }

  return message;
}

Connection::Connection(const std::shared_ptr<crypto::Ecc> &keys,
                       const std::shared_ptr<messages::Queue> &queue,
                       const std::shared_ptr<tcp::socket> &socket,
                       UnpairingCallback unpairing_callback)
    : ::neuro::networking::Connection::Connection(queue),
      _header(sizeof(HeaderPattern), 0),
      _buffer(128, 0),
      _socket(socket),
      _keys(keys),
      _unpairing_callback(unpairing_callback) {
  assert(_socket != nullptr);
}

bool Connection::match_remote_key_pub(const messages::KeyPub &key_pub) const {
  assert(!!_remote_pub_key);
  crypto::EccPub ecc_pub;
  ecc_pub.load(reinterpret_cast<const uint8_t *>(key_pub.raw_data().data()),
               key_pub.raw_data().size());
  return ecc_pub == *_remote_pub_key;
}

std::shared_ptr<const tcp::socket> Connection::socket() const {
  return _socket;
}

void Connection::read_header() {
  boost::asio::async_read(
      *_socket, boost::asio::buffer(_header.data(), _header.size()),
      [_this = ptr()](const boost::system::error_code &error,
                      std::size_t bytes_read) {
        if (error) {
          LOG_ERROR << _this << " " << __LINE__ << " Killing connection "
                    << error;
          _this->terminate();
          return;
        }
        const auto header_pattern =
            reinterpret_cast<HeaderPattern *>(_this->_header.data());
        _this->read_body(header_pattern->size);
      });
}

void Connection::read_body(std::size_t body_size) {
  assert(!!_remote_pub_key);  // Under derived class responsibility
  _buffer.resize(body_size);
  boost::asio::async_read(
      *_socket, boost::asio::buffer(_buffer.data(), _buffer.size()),
      [_this = ptr()](const boost::system::error_code &error,
                      std::size_t bytes_read) {
        if (error) {
          LOG_ERROR << _this << " " << __LINE__ << " Killing connection "
                    << error;
          _this->terminate();
          return;
        }

        auto message = Packet::deserialize(_this->_remote_pub_key,
                                           _this->_header, _this->_buffer);
        if (!message) {
          LOG_ERROR << _this << " " << __LINE__ << " Killing connection ";
          _this->terminate();
          return;
        }

        LOG_DEBUG << "\033[1;32mMessage received: " << *message << "\033[0m";

        if (!_this->_queue->publish(message)) {
          LOG_ERROR << "Failed to publish message.";
          _this->terminate();
          return;
        }
        _this->read_header();
      });
}

bool Connection::send(const messages::Message &message) {
  LOG_DEBUG << "\033[1;34mSending message: " << message << "<<\033[0m\n";
  auto header_tcp = std::make_shared<Buffer>(sizeof(HeaderPattern), 0);
  auto body_tcp = std::make_shared<Buffer>();
  if (!Packet::serialize(*_keys, message, *header_tcp, *body_tcp)) {
    LOG_WARNING << "Failed to serialize message.";
    return false;
  }
  return send(header_tcp, body_tcp);
}

bool Connection::send(const std::shared_ptr<Buffer> &header_tcp,
                      const std::shared_ptr<Buffer> &body_tcp) {
  assert(header_tcp != nullptr);
  assert(body_tcp != nullptr);
  boost::asio::async_write(
      *_socket, boost::asio::buffer(header_tcp->data(), header_tcp->size()),
      [_this = ptr(), header_tcp, body_tcp](
          const boost::system::error_code &error,
          std::size_t bytes_transferred) {
        if (error) {
          LOG_ERROR << "Could not send message";
          LOG_ERROR << _this << " " << __LINE__ << " Killing connection "
                    << error;
          _this->terminate();
          return;
        }
        boost::asio::async_write(
            *(_this->_socket),
            boost::asio::buffer(body_tcp->data(), body_tcp->size()),
            [_this, body_tcp](const boost::system::error_code &error,
                              std::size_t bytes_transferred) {
              if (error) {
                LOG_ERROR << "Could not send message";
                LOG_ERROR << _this << " " << __LINE__ << " Killing connection "
                          << error;
                _this->terminate();
              }
            });
      });
  return true;
}

void Connection::start(PairingCallback pairing_callback) {
  assert(!!_remote_pub_key);
  auto buffer = _remote_pub_key->save();
  pairing_callback(ptr(), buffer);
  read_header();
}

void Connection::close() { _socket->close(); }

messages::Peer Connection::build_peer(const IP &ip, Port port,
                                     const crypto::EccPub &ecc_pub) const {
  messages::Peer peer;
  peer.set_endpoint(ip.to_string());
  peer.set_port(port);
  auto key_pub = peer.mutable_key_pub();
  key_pub->set_type(messages::KeyType::ECP256K1);
  const auto tmp = ecc_pub.save();
  key_pub->set_raw_data(tmp.data(), tmp.size());
  return peer;
}

messages::Peer Connection::build_remote_peer() const {
  assert(!!_remote_pub_key);
  return build_peer(remote_ip(), remote_port(), *_remote_pub_key);
}

messages::Peer Connection::build_local_peer() const {
  return build_peer(local_ip(), local_port(), _keys->public_key());
}

void Connection::terminate() {
  close();
  if (!!_remote_pub_key) {
    _unpairing_callback(_remote_pub_key->save());
  }
}

IP Connection::remote_ip() const {
  const auto endpoint = _socket->remote_endpoint();
  return endpoint.address();
}

Port Connection::remote_port() const {
  const auto endpoint = _socket->remote_endpoint();
  return static_cast<Port>(endpoint.port());
}

IP Connection::local_ip() const {
  const auto endpoint = _socket->local_endpoint();
  return endpoint.address();
}

Port Connection::local_port() const {
  const auto endpoint = _socket->local_endpoint();
  return static_cast<Port>(endpoint.port());
}

Connection::~Connection() {
  terminate();
  LOG_DEBUG << this << " Connection killed";
}

}  // namespace tcp
}  // namespace networking
}  // namespace neuro
