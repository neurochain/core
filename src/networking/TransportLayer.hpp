#ifndef NEURO_SRC_NETWORKING_TRANSPORT_LAYER_HPP
#define NEURO_SRC_NETWORKING_TRANSPORT_LAYER_HPP

#include <thread>

#include "common/logger.hpp"
#include "common/types.hpp"
#include "crypto/Ecc.hpp"
#include "messages/Peer.hpp"
#include "messages/Peers.hpp"
#include "messages/Queue.hpp"
#include "networking/Connection.hpp"

namespace neuro {
namespace messages {
class Queue;
}  // namespace messages

namespace crypto {
class Ecc;
}  // namespace crypto

namespace networking {

class TransportLayer {
 public:
  enum class SendResult { FAILED, ONE_OR_MORE_SENT, ALL_GOOD };

 protected:
  messages::Queue* _queue;
  messages::Peers* _peers;
  crypto::Ecc* _keys;

 public:
  TransportLayer(messages::Queue* queue, messages::Peers* peers,
                 crypto::Ecc* keys);

  virtual SendResult send(
      const std::shared_ptr<messages::Message> message) const = 0;
  virtual bool send_unicast(
      const std::shared_ptr<messages::Message> message) const = 0;
  virtual std::size_t peer_count() const = 0;
  virtual bool terminate(const Connection::ID id) = 0;
  virtual std::optional<messages::Peer*> find_peer(const Connection::ID id) = 0;
  virtual Port listening_port() const = 0;
  virtual bool connect(std::shared_ptr<messages::Peer> peer) = 0;
  virtual void clean_old_connections(int delta) = 0;

  virtual ~TransportLayer() = default;
  virtual void join() = 0;
};

}  // namespace networking

}  // namespace neuro

#endif /* NEURO_SRC_TRANSPORT_LAYER_HPP */
