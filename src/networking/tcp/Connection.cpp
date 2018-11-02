#include "networking/tcp/Connection.hpp"
#include "common/logger.hpp"
#include "common/types.hpp"
#include "messages/Queue.hpp"
#include "networking/tcp/Tcp.hpp"

namespace neuro {
namespace networking {
namespace tcp {

Connection::Ptr Connection::create(
    const ID id, networking::TransportLayer::ID transport_layer_id,
    std::shared_ptr<messages::Queue> queue, std::shared_ptr<tcp::socket> socket,
    std::shared_ptr<messages::Peer> remote_peer, const bool from_remote) {
  return Ptr{new Connection(id, transport_layer_id, queue, socket, remote_peer,
                            from_remote)};
}

Buffer &Connection::header() { return _header; }

Buffer &Connection::buffer() { return _buffer; }

std::shared_ptr<tcp::socket> Connection::socket() { return _socket; }

std::shared_ptr<messages::Peer> Connection::remote_peer() const {
  return _remote_peer;
}

void Connection::set_listen_port(::google::protobuf::int32 port) {
  _listen_port = port;
}

void Connection::read() { read_header(); }

void Connection::read_header() {
  auto self = shared_from_this();
  boost::asio::async_read(
      *_socket, boost::asio::buffer(_header.data(), _header.size()),
      [&self](const boost::system::error_code &error, std::size_t bytes_read) {
        if (error) {
          LOG_ERROR << self.get() << " " << __LINE__ << " Killing connection "
                    << error;
          self->terminate();
          return;
        }

        auto header_pattern =
            reinterpret_cast<HeaderPattern *>(self->header().data());
        self->buffer().resize(header_pattern->size);
        self->read_body();
      });
}

void Connection::read_body() {
  auto self = shared_from_this();
  boost::asio::async_read(
      *_socket, boost::asio::buffer(_buffer.data(), _buffer.size()),
      [&self](const boost::system::error_code &error, std::size_t bytes_read) {
        if (error) {
          LOG_ERROR << self.get() << " " << __LINE__ << " Killing connection "
                    << error;
          self->terminate();
          return;
        }

        const auto header_pattern =
            reinterpret_cast<HeaderPattern *>(self->header().data());

        auto message = std::make_shared<messages::Message>();
        messages::from_buffer(self->buffer(), message.get());

        auto header = message->mutable_header();

        header->mutable_peer()->CopyFrom(*self->remote_peer());
        auto signature = header->mutable_signature();
        signature->set_type(messages::Hash::SHA256);
        signature->set_data(header_pattern->signature,
                            sizeof(header_pattern->signature));

        // validate the MessageVersion from the header
        if (header->version() != neuro::MessageVersion) {
          LOG_ERROR << " MessageVersion not corresponding: "
                    << neuro::MessageVersion << " (mine) vs "
                    << header->version() << " )";
          self->terminate();
          return;
        }

        for (const auto &body : message->bodies()) {
          const auto type = get_type(body);
          LOG_DEBUG << self.get() << " read_body TYPE " << type;
          if (type == messages::Type::kHello) {
            auto hello = body.hello();

            if (hello.has_listen_port()) {
              self->set_listen_port(hello.listen_port());
            }

            if (!self->remote_peer()->has_key_pub()) {
              LOG_INFO << "Updating peer with hello key pub";
              self->remote_peer()->mutable_key_pub()->CopyFrom(hello.key_pub());
            }
          }
        }
        std::cout << "\033[1;32mMessage received: " << *message << "\033[0m"
                  << std::endl;

        if (!self->remote_peer()->has_key_pub()) {
          LOG_ERROR << "Not Key pub set";
          self->read_header();
          return;
        }
        crypto::EccPub ecc_pub;
        const auto key_pub = self->remote_peer()->key_pub();

        ecc_pub.load(
            reinterpret_cast<const uint8_t *>(key_pub.raw_data().data()),
            key_pub.raw_data().size());

        const auto check =
            ecc_pub.verify(self->buffer(), header_pattern->signature,
                           sizeof(header_pattern->signature));

        if (!check) {
          LOG_ERROR << "Bad signature, dropping message";
          self->read_header();
          return;
        }

        self->queue()->publish(message);
        self->read_header();
      });
}

bool Connection::send(const Buffer &message) {
  auto self = shared_from_this();
  boost::asio::async_write(*_socket,
                           boost::asio::buffer(message.data(), message.size()),
                           [&self](const boost::system::error_code &error,
                                   std::size_t bytes_transferred) {
                             if (error) {
                               LOG_ERROR << "Could not send message";
                               LOG_ERROR << self.get() << " " << __LINE__
                                         << " Killing connection " << error;
                               self->terminate();
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
