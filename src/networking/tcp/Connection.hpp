#ifndef NEURO_SRC_NETWORKING_TCP_CONNECTION_HPP
#define NEURO_SRC_NETWORKING_TCP_CONNECTION_HPP

#include <stdio.h>
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <cmath>
#include <iostream>

#include "common/types.hpp"
#include "crypto/Ecc.hpp"
#include "messages.pb.h"
#include "messages/Queue.hpp"
#include "networking/Connection.hpp"
#include "networking/TransportLayer.hpp"
#include "networking/tcp/HeaderPattern.hpp"

namespace neuro {
namespace networking {
namespace tcp {
using boost::asio::ip::tcp;

class Connection : public networking::Connection {
 public:
  using PairingCallback = std::function<void(
      const std::shared_ptr<Connection> &paired_connection, const Buffer &id)>;
  using UnpairingCallback = std::function<void(const Buffer &id)>;

 public:
  Connection(const std::shared_ptr<messages::Queue> &queue)
  : networking::Connection(queue) {}
  virtual bool send(const messages::Message &message) = 0;
  virtual void close() = 0;
};

template <class Derived>
class ConnectionBase : public Connection,
                   public std::enable_shared_from_this<Derived> {
 protected:
  Buffer _header;
  Buffer _buffer;
  std::shared_ptr<tcp::socket> _socket;
  std::shared_ptr<crypto::Ecc> _keys;
  std::optional<crypto::EccPub> _remote_pub_key;
  UnpairingCallback _unpairing_callback;

  class Packet {
   public:
    static bool serialize(const crypto::Ecc& keys,
                          const messages::Message& message, Buffer& header_tcp,
                          Buffer& body_tcp);
    static std::shared_ptr<messages::Message> deserialize(
        const std::optional<crypto::EccPub>& pub_key, const Buffer& header_tcp,
        const Buffer& body_tcp);
  };

private:
 void read_body(std::size_t body_size);
 messages::Peer build_peer(const IP& ip, Port port,
                           const crypto::EccPub& ecc_pub) const;
 messages::Peer build_remote_peer() const;
 messages::Peer build_local_peer() const;
 bool send(const std::shared_ptr<Buffer>& header_tcp,
           const std::shared_ptr<Buffer>& body_tcp);

protected:
 ConnectionBase(const std::shared_ptr<crypto::Ecc> &keys,
                const std::shared_ptr<messages::Queue> &queue,
                const std::shared_ptr<tcp::socket> &socket,
                UnpairingCallback unpairing_callback);
 void start(PairingCallback pairing_callback);
 void read_header();
 virtual void read_handshake_message_body(PairingCallback pairing_callback,
                                          std::size_t body_size) = 0;
 void terminate();

public:
 std::shared_ptr<const tcp::socket> socket() const;
 bool send(const messages::Message& message);
 IP remote_ip() const;
 Port remote_port() const;
 IP local_ip() const;
 Port local_port() const;
 void close();
 ~ConnectionBase();
};

template <class D>
bool ConnectionBase<D>::Packet::serialize(const crypto::Ecc &keys,
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

template <class D>
std::shared_ptr<messages::Message> ConnectionBase<D>::Packet::deserialize(
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

  if (!!pub_key) {
    crypto::EccPub ecc_pub;
    ecc_pub.load(header->key_pub());
    if (ecc_pub.save() != pub_key->save()) {
      LOG_ERROR << "Unexpected peer public key!";
      message.reset();
      return message;
    }
    if (pub_key->verify(body_tcp, header_pattern->signature,
                        sizeof(header_pattern->signature)) == false) {
      LOG_ERROR << "Bad signature, dropping message";
      message.reset();
      return message;
    }
  }

  return message;
}

template <class D>
ConnectionBase<D>::ConnectionBase(const std::shared_ptr<crypto::Ecc> &keys,
                                  const std::shared_ptr<messages::Queue> &queue,
                                  const std::shared_ptr<tcp::socket> &socket,
                                  UnpairingCallback unpairing_callback)
    : ::neuro::networking::tcp::Connection(queue),
      _header(sizeof(HeaderPattern), 0),
      _buffer(128, 0),
      _socket(socket),
      _keys(keys),
      _unpairing_callback(unpairing_callback) {
  assert(_socket != nullptr);
}

template <class D>
std::shared_ptr<const tcp::socket> ConnectionBase<D>::socket() const {
  return _socket;
}

template <class D>
void ConnectionBase<D>::read_header() {
  auto _this = ConnectionBase<D>::shared_from_this();
  boost::asio::async_read(
      *_socket, boost::asio::buffer(_header.data(), _header.size()),
      [_this](const boost::system::error_code &error, std::size_t bytes_read) {
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

template <class D>
void ConnectionBase<D>::read_body(std::size_t body_size) {
  assert(!!_remote_pub_key);  // Under derived class responsibility
  _buffer.resize(body_size);
  auto _this = ConnectionBase<D>::shared_from_this();
  boost::asio::async_read(
      *_socket, boost::asio::buffer(_buffer.data(), _buffer.size()),
      [_this](const boost::system::error_code &error, std::size_t bytes_read) {
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

template <class D>
bool ConnectionBase<D>::send(const messages::Message &message) {
  LOG_DEBUG << "\033[1;34mSending message: " << message << "<<\033[0m\n";
  auto header_tcp = std::make_shared<Buffer>(sizeof(HeaderPattern), 0);
  auto body_tcp = std::make_shared<Buffer>();
  if (!Packet::serialize(*_keys, message, *header_tcp, *body_tcp)) {
    LOG_WARNING << "Failed to serialize message.";
    return false;
  }
  return send(header_tcp, body_tcp);
}

template <class D>
bool ConnectionBase<D>::send(const std::shared_ptr<Buffer> &header_tcp,
                             const std::shared_ptr<Buffer> &body_tcp) {
  assert(header_tcp != nullptr);
  assert(body_tcp != nullptr);
  auto _this = ConnectionBase<D>::shared_from_this();
  boost::asio::async_write(
      *_socket, boost::asio::buffer(header_tcp->data(), header_tcp->size()),
      [_this, header_tcp, body_tcp](const boost::system::error_code &error,
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

template <class D>
void ConnectionBase<D>::start(PairingCallback pairing_callback) {
  assert(!!_remote_pub_key);
  auto buffer = _remote_pub_key->save();
  pairing_callback(ConnectionBase<D>::shared_from_this(), buffer);
  read_header();
}

template <class D>
void ConnectionBase<D>::close() {
  _socket->close();
}

template <class D>
messages::Peer ConnectionBase<D>::build_peer(
    const IP &ip, Port port, const crypto::EccPub &ecc_pub) const {
  messages::Peer peer;
  peer.set_endpoint(ip.to_string());
  peer.set_port(port);
  auto key_pub = peer.mutable_key_pub();
  key_pub->set_type(messages::KeyType::ECP256K1);
  const auto tmp = ecc_pub.save();
  key_pub->set_raw_data(tmp.data(), tmp.size());
  return peer;
}

template <class D>
messages::Peer ConnectionBase<D>::build_remote_peer() const {
  assert(!!_remote_pub_key);
  return build_peer(remote_ip(), remote_port(), *_remote_pub_key);
}

template <class D>
messages::Peer ConnectionBase<D>::build_local_peer() const {
  return build_peer(local_ip(), local_port(), _keys->public_key());
}

template <class D>
void ConnectionBase<D>::terminate() {
  close();
  if (!!_remote_pub_key) {
    _unpairing_callback(_remote_pub_key->save());
  }
}

template <class D>
IP ConnectionBase<D>::remote_ip() const {
  const auto endpoint = _socket->remote_endpoint();
  return endpoint.address();
}

template <class D>
Port ConnectionBase<D>::remote_port() const {
  const auto endpoint = _socket->remote_endpoint();
  return static_cast<Port>(endpoint.port());
}

template <class D>
IP ConnectionBase<D>::local_ip() const {
  const auto endpoint = _socket->local_endpoint();
  return endpoint.address();
}

template <class D>
Port ConnectionBase<D>::local_port() const {
  const auto endpoint = _socket->local_endpoint();
  return static_cast<Port>(endpoint.port());
}

template <class D>
ConnectionBase<D>::~ConnectionBase() {
  terminate();
  LOG_DEBUG << this << " Connection killed";
}

}  // namespace tcp
}  // namespace networking
}  // namespace neuro

#endif /* NEURO_SRC_NETWORKING_CONNECTION_HPP */
