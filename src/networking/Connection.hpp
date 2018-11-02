#ifndef NEURO_SRC_NETWORKING_CONNECTION_HPP
#define NEURO_SRC_NETWORKING_CONNECTION_HPP

#include <memory>

#include "common/types.hpp"
#include "networking/TransportLayer.hpp"

namespace neuro {
namespace networking {

class Connection : public std::enable_shared_from_this<Connection> {
 public:
  using ID = uint16_t;

 protected:
  ID _id;
  TransportLayer::ID _transport_layer_id;
  std::shared_ptr<messages::Queue> _queue;

 protected:
  Connection(const ID id, TransportLayer::ID transport_layer_id,
             std::shared_ptr<messages::Queue> queue)
      : _id(id), _transport_layer_id(transport_layer_id), _queue(queue) {}

 public:
  virtual Buffer& header() = 0;
  virtual Buffer& buffer() = 0;
  virtual void read_header() = 0;
  virtual void read_body() = 0;
  virtual void terminate() = 0;
  virtual std::shared_ptr<messages::Peer> remote_peer() const = 0;
  virtual void set_listen_port(::google::protobuf::int32 port) = 0;
  std::shared_ptr<messages::Queue> queue() const;

  ID id() const;
  TransportLayer::ID transport_layer_id() const;
};

}  // namespace networking
}  // namespace neuro

#endif /* NEURO_NETWORKING_CONNECTION_HPP */
