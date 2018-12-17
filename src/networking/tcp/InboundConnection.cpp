#include "networking/tcp/InboundConnection.hpp"
#include "crypto/Ecc.hpp"

namespace neuro {
namespace networking {
namespace tcp {

void InboundConnection::read_hello(PairingCallback pairing_callback) {
  boost::asio::async_read(
      *_socket, boost::asio::buffer(_header.data(), _header.size()),
      [this, _this = ptr(), pairing_callback](
          const boost::system::error_code& error, std::size_t bytes_read) {
        if (error) {
          LOG_ERROR
              << "Error reading outbound connection acknowledgment header.";
          return;
        }
        const auto header_pattern =
            reinterpret_cast<HeaderPattern*>(this->_header.data());
        this->read_handshake_message_body(pairing_callback,
                                          header_pattern->size);
      });
}

void InboundConnection::read_handshake_message_body(
    PairingCallback pairing_callback, std::size_t body_size) {
  _buffer.resize(body_size);
  boost::asio::async_read(
      *_socket, boost::asio::buffer(_buffer.data(), _buffer.size()),
      [this, _this = ptr(), pairing_callback](
          const boost::system::error_code& error, std::size_t bytes_read) {
        if (error) {
          LOG_ERROR << "Error reading outbound connection acknowledgment body.";
          return;
        }
        assert(!this->_remote_pub_key);
        auto message = Packet::deserialize(this->_remote_pub_key, this->_header,
                                           this->_buffer);
        if (!message) {
          LOG_ERROR << "Failed to deserialize hello message.";
          return;
        }
        LOG_DEBUG << "\033[1;32mMessage received: " << *message << "\033[0m";

        if (!message->has_header()) {
          LOG_ERROR << "Missing header on hello message.";
          return;
        }

        std::size_t count_hello = 0;
        for (const auto& body : message->bodies()) {
          const auto type = get_type(body);
          if (type != messages::Type::kHello) {
            LOG_WARNING << "Hello message expected.";
            return;
          }
          if (count_hello++) {
            LOG_WARNING << "Only one hello body expected.";
            return;
          }
          crypto::EccPub ecc_pub;
          ecc_pub.load(body.hello().key_pub());
          this->_remote_pub_key = ecc_pub.save();
        }
        if (!count_hello) {
          LOG_WARNING << "At least one hello body expected.";
          return;
        }
        pairing_callback(_this, this->_remote_pub_key->save());

        messages::Message world_message;
        auto world = world_message.add_bodies()->mutable_world();
        auto header_reply = world_message.mutable_header();
        messages::fill_header_reply(message->header(), header_reply);
        Buffer key_pub_buffer;
        _keys->public_key().save(&key_pub_buffer);
        auto key_pub = world->mutable_key_pub();
        key_pub->set_type(messages::KeyType::ECP256K1);
        key_pub->set_hex_data(key_pub_buffer.str());

        _this->send(world_message);
      });
}

InboundConnection::InboundConnection(
    const std::shared_ptr<crypto::Ecc>& keys,
    const std::shared_ptr<messages::Queue>& queue,
    const std::shared_ptr<tcp::socket>& socket,
    PairingCallback pairing_callback, UnpairingCallback unpairing_callback)
    : Connection(keys, queue, socket, unpairing_callback) {
  read_hello(pairing_callback);
}

}  // namespace tcp
}  // namespace networking
}  // namespace neuro
