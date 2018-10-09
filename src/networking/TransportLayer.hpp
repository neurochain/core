#ifndef NEURO_SRC_NETWORKING_TRANSPORT_LAYER_HPP
#define NEURO_SRC_NETWORKING_TRANSPORT_LAYER_HPP

#include <thread>

#include "common/logger.hpp"
#include "common/types.hpp"

namespace neuro {
namespace messages {
class Queue;
}  // namespace messages

namespace crypto {
class Ecc;
}  // namespace crypto

namespace networking {
class Networking;

class TransportLayer {
 public:
  using ID = int16_t;

 protected:
  std::shared_ptr<messages::Queue> _queue;
  std::shared_ptr<crypto::Ecc> _keys;
  ID _id;
  std::thread _thread;

 protected:
  virtual void _run() = 0;
  virtual void _stop() = 0;

 public:
  TransportLayer(std::shared_ptr<messages::Queue> queue,
                 std::shared_ptr<crypto::Ecc> keys);

  virtual bool send(const std::shared_ptr<messages::Message> message,
                    ProtocolType type) = 0;
  virtual bool send_unicast(const std::shared_ptr<messages::Message> message,
                            ProtocolType type) = 0;
  ID id() const;
  void id(const ID id);
  void run();
  void stop();
  void join();
  virtual ~TransportLayer();
};

}  // namespace networking

}  // namespace neuro

#endif /* NEURO_SRC_TRANSPORT_LAYER_HPP */
