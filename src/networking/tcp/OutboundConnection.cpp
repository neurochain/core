#include "networking/tcp/OutboundConnection.hpp"
#include "crypto/Ecc.hpp"

namespace neuro {
namespace networking {
namespace tcp {

OutboundConnection::OutboundConnection(
    const std::shared_ptr<crypto::Ecc>& keys,
    const std::shared_ptr<messages::Queue>& queue,
    const std::shared_ptr<tcp::socket>& socket, const messages::Peer& peer,
    UnpairingCallback unpairing_callback)
    : ConnectionBase(keys, queue, socket, unpairing_callback) {
  assert(!!peer.has_endpoint());
  assert(remote_ip().to_string() == peer.endpoint());
  assert(!!peer.has_key_pub());
  _remote_pub_key = crypto::EccPub{};
  _remote_pub_key->load(peer.key_pub());
}

messages::Message OutboundConnection::build_hello_message() const {
  messages::Message message;
  messages::fill_header(message.mutable_header());
  auto header = message.mutable_header();
  auto key_pub = header->mutable_key_pub();
  key_pub->set_type(messages::KeyType::ECP256K1);
  const auto tmp = _keys->public_key().save();
  key_pub->set_raw_data(tmp.data(), tmp.size());
  auto hello = message.add_bodies()->mutable_hello();
  hello->set_listen_port(local_port());
  return message;
}

void OutboundConnection::read_handshake_message_body(
    PairingCallback pairing_callback, std::size_t body_size) {
  _buffer.resize(body_size);
  auto _this = shared_from_this();
  boost::asio::async_read(
      *_socket, boost::asio::buffer(_buffer.data(), _buffer.size()),
      [_this, pairing_callback](const boost::system::error_code& error,
                                std::size_t bytes_read) {
        if (!!error) {
          LOG_ERROR << "Error reading outbound connection acknowledgment body: "
                    << error;
          return;
        }
        auto message = Packet::deserialize(_this->_remote_pub_key,
                                           _this->_header, _this->_buffer);
        if (!message) {
          LOG_ERROR << "Failed to deserialize acknowledgment body.";
          return;
        }

        LOG_DEBUG << "\033[1;32mMessage received: " << *message << "\033[0m";

        if (!message->has_header()) {
          LOG_ERROR << "Missing header on hello message.";
          return;
        }

        std::size_t count_world = 0;
        for (const auto& body : message->bodies()) {
          const auto type = get_type(body);
          if (type != messages::Type::kWorld) {
            LOG_WARNING << "World message expected.";
            return;
          }
          if (count_world++) {
            LOG_WARNING << "Only one world body expected.";
            return;
          }
        }
        if (!count_world) {
          LOG_WARNING << "At least one hello body expected.";
          return;
        }

        crypto::EccPub ecc_pub;
        ecc_pub.load(message->header().key_pub());
        assert(!!_this->_remote_pub_key);
        if (_this->_remote_pub_key->save() != ecc_pub.save()) {
          LOG_ERROR << "Unexpected signature specified.";
          return;
        }

        _this->start(pairing_callback);
      });
}

void OutboundConnection::read_ack(PairingCallback pairing_callback) {
  auto _this = shared_from_this();
  boost::asio::async_read(
      *_socket, boost::asio::buffer(_header.data(), _header.size()),
      [_this, pairing_callback](const boost::system::error_code& error,
                                std::size_t bytes_read) {
        if (!!error) {
          LOG_ERROR
              << "Error reading outbound connection acknowledgment header: "
              << error;
          return;
        }
        const auto header_pattern =
            reinterpret_cast<HeaderPattern*>(_this->_header.data());
        _this->read_handshake_message_body(pairing_callback,
                                           header_pattern->size);
      });
}

void OutboundConnection::handshake_init(PairingCallback pairing_callback) {
  auto hello_message = build_hello_message();
  send(hello_message);
  read_ack(pairing_callback);
}

}  // namespace tcp
}  // namespace networking
}  // namespace neuro
