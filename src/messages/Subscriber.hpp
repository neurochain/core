#ifndef NEURO_SRC_MESSAGES_SUBSCRIBER_HPP
#define NEURO_SRC_MESSAGES_SUBSCRIBER_HPP

#include <set>
#include <typeindex>
#include <typeinfo>
#include "crypto/Hash.hpp"
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
  std::unordered_set<std::shared_ptr<Buffer>> _seen_messages_hash;
  std::map<std::time_t, std::shared_ptr<Buffer>> _message_hash_by_ts;

 public:
  Subscriber(std::shared_ptr<Queue> queue)
      : _queue(queue), _callbacks_by_type(Body::kBodyCount) {
    _queue->subscribe(this);
  }

  void subscribe(const Type type, const Callback &callback) {
    _callbacks_by_type[type].emplace_back(callback);
  }

  void unsubscribe() { _queue->unsubscribe(this); }

  bool is_new_body(const std::time_t current_time, const messages::Body &body) {
    // TOOD move this to the queue
    Buffer serialized_body;
    to_buffer(body, &serialized_body);

    const auto hash =
        std::make_shared<Buffer>(crypto::hash_sha3_256(serialized_body));
    const auto &[it, is_emplaced] = _seen_messages_hash.emplace(hash);
    if (!is_emplaced) {
      LOG_TRACE << "message already seened " << body;
      return false;
    }

    _message_hash_by_ts.emplace(std::piecewise_construct,
                                std::forward_as_tuple(current_time),
                                std::forward_as_tuple(hash));
    LOG_TRACE << "new message " << body;
    _message_hash_by_ts.erase(
        _message_hash_by_ts.begin(),
        _message_hash_by_ts.lower_bound(current_time - MESSAGE_TTL));

    return true;
  }

  void handler(std::shared_ptr<const Message> message) {
    std::lock_guard<std::mutex> lock_handler(_mutex_handler);
    const auto time = std::time(nullptr);
    for (const auto &body : message->bodies()) {
      const auto type = get_type(body);
      for (const auto &cb : _callbacks_by_type[type]) {
        if (is_new_body(time, body)) {
          cb(message->header(), body);
        }
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
