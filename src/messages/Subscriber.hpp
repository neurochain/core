#ifndef NEURO_SRC_MESSAGES_SUBSCRIBER_HPP
#define NEURO_SRC_MESSAGES_SUBSCRIBER_HPP

#include <set>
#include <typeindex>
#include <typeinfo>
#include "messages.pb.h"
#include "messages/Queue.hpp"

namespace neuro {
namespace messages {

class Subscriber {
 public:
  using Callback = std::function<void(const Header &header, const Body &body)>;

 private:
  mutable std::mutex _mutex_handler;
  std::shared_ptr<Queue> _queue;
  std::vector<std::vector<Callback>> _callbacks_by_type;

 public:
  Subscriber(std::shared_ptr<Queue> queue)
      : _queue(queue), _callbacks_by_type(Body::kBodyCount) {
    _queue->subscribe(this);
  }

  void subscribe(const Type type, const Callback &callback) {
    _callbacks_by_type[type].emplace_back(callback);
  }

  void unsubscribe() { _queue->unsubscribe(this); }

  void handler(std::shared_ptr<const Message> message) {
    std::lock_guard<std::mutex> lock_handler(_mutex_handler);
    for (const auto &body : message->bodies()) {
      const auto type = get_type(body);
      for (const auto &cb : _callbacks_by_type[type]) {
        cb(message->header(), body);
      }
    }
  }

  ~Subscriber() {
    std::lock_guard<std::mutex> lock_handler(_mutex_handler);
    LOG_DEBUG << "Subscriber unsubscribing " << this;
    _queue->unsubscribe(this);
  }
};

}  // namespace messages
}  // namespace neuro

#endif /* NEURO_SRC_MESSAGES_SUBSCRIBER_HPP */
