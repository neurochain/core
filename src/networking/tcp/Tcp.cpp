#include <chrono>
#include <sstream>
#include <thread>
#include <tuple>

#include "crypto/Ecc.hpp"
#include "common/logger.hpp"
#include "networking/Networking.hpp"
#include "networking/tcp/HeaderPattern.hpp"
#include "networking/tcp/Tcp.hpp"

namespace neuro {
namespace networking {
using namespace std::chrono_literals;

Tcp::Tcp(std::shared_ptr<messages::Queue> queue,
         std::shared_ptr<crypto::Ecc> keys)
    : TransportLayer(queue, keys), _io_service(),
      _resolver(_io_service), _current_connection_id(0) {}

bool Tcp::connect(const bai::tcp::endpoint host, const Port port) {
  return false; // TODO
}

void Tcp::connect(std::shared_ptr<messages::Peer> peer) {
  bai::tcp::resolver resolver(_io_service);
  bai::tcp::resolver::query query(peer->endpoint(),
                                  std::to_string(peer->port()));
  bai::tcp::resolver::iterator endpoint_iterator = resolver.resolve(query);

  auto socket = std::make_shared<bai::tcp::socket>(_io_service);
  auto handler = [this, socket, peer](boost::system::error_code error,
                                      bai::tcp::resolver::iterator iterator) {
    this->new_connection(socket, error, peer, false);
  };
  boost::asio::async_connect(*socket, endpoint_iterator, handler);
}

void Tcp::connect(const std::string &host, const std::string &service) {}

void Tcp::accept(const Port port) {
  auto acceptor = std::make_shared<bai::tcp::acceptor>(
      _io_service, bai::tcp::endpoint(bai::tcp::v4(), port));
  accept(acceptor, port);
}

void Tcp::accept(std::shared_ptr<bai::tcp::acceptor> acceptor,
                 const Port port) {
  _listening_port = port;

  auto socket = std::make_shared<bai::tcp::socket>(acceptor->get_io_service());
  acceptor->async_accept
      (*socket,
       [this, acceptor, socket, port](const boost::system::error_code &error) {
        const auto remote_endpoint = socket->remote_endpoint().address().to_string();
        
        auto peer = std::make_shared<messages::Peer>();
        peer->set_endpoint(remote_endpoint);
        peer->set_port(socket->remote_endpoint().port());
        peer->set_status(messages::Peer::CONNECTING);
        peer->set_transport_layer_id(_id);
			   
        this->new_connection(socket, error, peer, true);
        this->accept(acceptor, port);
      });
}

Port Tcp::listening_port() const { return _listening_port; }
IP Tcp::local_ip() const { return _local_ip; }

void Tcp::new_connection(std::shared_ptr<bai::tcp::socket> socket,
                         const boost::system::error_code &error,
                         std::shared_ptr<messages::Peer> peer,
                         const bool from_remote) {
  std::lock_guard<std::mutex> lock_queue(_connection_mutex);

  auto message = std::make_shared<messages::Message>();
  auto header = message->mutable_header();
  auto peer_tmp = header->mutable_peer();
  auto body = message->add_bodies();

  if (!error) {
    _current_connection_id++;
    peer->set_connection_id(_current_connection_id);
    
    auto r = _connections.emplace(
        std::piecewise_construct, std::forward_as_tuple(_current_connection_id),
        std::forward_as_tuple(_current_connection_id, this->id(), _queue,
                              socket, peer,
                              from_remote));

    auto connection_ready = body->mutable_connection_ready();
    
    connection_ready->set_from_remote(from_remote);

    peer_tmp->CopyFrom(*peer);
    _queue->publish(message);
    r.first->second.read();
  } else {
    LOG_WARNING << "Could not create new connection to " << *peer;

    body->mutable_connection_closed();
    peer_tmp->CopyFrom(*peer);
    _queue->publish(message);
  }
}

void Tcp::_run() {
  LOG_INFO << "Starting io_service";
  boost::system::error_code ec;
  {
    std::lock_guard<std::mutex> m(_stopping_mutex);
    if (_stopping) {
      return;
    }
  }
  _io_service.run(ec);
  if (ec) {
    LOG_ERROR << "service run failed (" << ec.message() << ")";
  }
}

void Tcp::_stop() {
  {
    std::lock_guard<std::mutex> m(_stopping_mutex);
    _stopping = true;
  }
  _io_service.stop();
  while (!_io_service.stopped()) {
    std::this_thread::sleep_for(10ms);
  }
}

bool Tcp::serialize(std::shared_ptr<const messages::Message> message,
                    const ProtocolType protocol_type,
                    Buffer *header_tcp,
                    Buffer *body_tcp) {

  messages::to_buffer(*message, body_tcp);

  /// validate that the size of the buffer can be written in
  /// tcp::HeaderPattern::size
  if (body_tcp->size() > (1 << (8 * sizeof(tcp::HeaderPattern::size)))) {
    LOG_ERROR << "Message is too big (" << body_tcp->size() << ")";
    return false;
  }

  auto header_pattern = reinterpret_cast<tcp::HeaderPattern *>(header_tcp->data());
  header_pattern->size = body_tcp->size();
  _keys->sign(body_tcp->data(), body_tcp->size(),
              reinterpret_cast<uint8_t *>(&header_pattern->signature));
  header_pattern->type = protocol_type;

  return true;
}

bool Tcp::send(std::shared_ptr<messages::Message> message,
               ProtocolType protocol_type) {
  std::lock_guard<std::mutex> lock_queue(_connection_mutex);

  if (_connections.size() == 0) {
    LOG_ERROR << "Could not send message because there is no connection";
    return false;
  }

  Buffer header_tcp;
  Buffer body_tcp;
  serialize(message, protocol_type, &header_tcp, &body_tcp);

  bool res = true;
  for (auto &connection : _connections) {
    res &= connection.second.send(header_tcp);
    res &= connection.second.send(body_tcp);
  }

  return res;
}

bool Tcp::send_unicast(std::shared_ptr<messages::Message> message,
                       ProtocolType protocol_type) {
  std::lock_guard<std::mutex> lock_queue(_connection_mutex);
  assert(message->header().has_peer());
  auto got = _connections.find(message->header().peer().connection_id());
  if (got == _connections.end()) {
    return false;
  }

  Buffer header_tcp;
  Buffer body_tcp;
  serialize(message, protocol_type, &header_tcp, &body_tcp);

  got->second.send(header_tcp);
  got->second.send(body_tcp);

  return true;
}

bool Tcp::disconnected(const Connection::ID id, std::shared_ptr<Peer> peer) {
  {
    std::lock_guard<std::mutex> lock_queue(_connection_mutex);
    auto got = _connections.find(id);
    if (got == _connections.end()) {
      LOG_ERROR << "Connection not found";
      return false;
    }
    _connections.erase(got);
  }

  return true;
}

Tcp::~Tcp() {
  std::lock_guard<std::mutex> lock_queue(_connection_mutex);
  _stop();
  LOG_DEBUG << "TCP Killing: " << this;
}

} // namespace networking
} // namespace neuro
