#ifndef NEURO_SRC_NETWORKING_TRANSPORT_LAYER_HPP
#define NEURO_SRC_NETWORKING_TRANSPORT_LAYER_HPP

#include <thread>

#include "common/logger.hpp"
#include "common/types.hpp"
#include "messages/Peers.hpp"

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
  using ID = int16_t;
  enum class SendResult {
    FAILED, ONE_OR_MORE_SENT, ALL_GOOD
  };

 protected:
  ID _id;
  messages::Queue* _queue;
  messages::Peers *_peers;
  std::shared_ptr<crypto::Ecc> _keys;

 public:
  TransportLayer(const ID id, messages::Queue* queue,
		 messages::Peers *peers, std::shared_ptr<crypto::Ecc> keys);

  virtual SendResult send(const std::shared_ptr<messages::Message> message) = 0;
  virtual bool send_unicast(const std::shared_ptr<messages::Message> message) = 0;
  virtual std::size_t peer_count() const = 0;

  ID id() const;
  virtual ~TransportLayer(){};
  virtual void join() = 0;
};

}  // namespace networking

}  // namespace neuro

#endif /* NEURO_SRC_TRANSPORT_LAYER_HPP */
