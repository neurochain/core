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
  using CallbackR = std::function<void(const Message &message)>;

 private:
  mutable std::mutex _mutex_handler;
  Queue *_queue;
  std::unordered_map<Message::ID, CallbackR> _callbacks_by_id;
  std::vector<std::vector<Callback>> _callbacks_by_type;
  std::unordered_set<Buffer> _seen_messages_hash;
  std::map<std::time_t, Buffer> _message_hash_by_ts;

 public:
  Subscriber(Queue *queue)
      : _queue(queue), _callbacks_by_type(Body::kBodyCount) {
    _queue->subscribe(this);
  }

  void subscribe(const Type type, const Callback &callback) {
    _callbacks_by_type[type].emplace_back(callback);
  }

  void subscribe(const Message::ID id, const CallbackR &callback) {
    _callbacks_by_id[id] = callback;
  }

  void unsubscribe() { _queue->unsubscribe(this); }

  bool is_new_body(const std::time_t current_time, const messages::Body &body) {
    // TODO move this to the queue
    Buffer serialized_body;
    to_buffer(body, &serialized_body);

    const auto hash = crypto::hash_sha3_256(serialized_body);
    const auto pair = _seen_messages_hash.emplace(hash);
    std::cout << "trax> msg hist " << body << " " << pair.second << std::endl;
    if (!pair.second) {
      return false;
    }

    _message_hash_by_ts.emplace(std::piecewise_construct,
                                std::forward_as_tuple(current_time),
                                std::forward_as_tuple(hash));

    for (auto it = _message_hash_by_ts.begin(),
              end = _message_hash_by_ts.lower_bound(current_time - MESSAGE_TTL);
         it != end;) {
      _seen_messages_hash.erase(it->second);
      it = _message_hash_by_ts.erase(it);
    }

    return true;
  }

  void handler(std::shared_ptr<const Message> message) {
    std::lock_guard<std::mutex> lock_handler(_mutex_handler);

    if (message->has_header() && message->header().has_id()) {
      const auto id = message->header().id();
      auto got = _callbacks_by_id.find(id);
      if (got != _callbacks_by_id.end()) {
        got->second(*message.get());
      }
    }

    const auto time = std::time(nullptr);
    for (const auto &body : message->bodies()) {
      const auto type = get_type(body);
        bool process{true};
        if (type == messages::Type::kTransaction ||
            type == messages::Type::kBlock) {
          process = is_new_body(time, body);
        }

        if (process) {
	  for (const auto &cb : _callbacks_by_type[type]) {
	    cb(message->header(), body);
	  }
        } else {
	  std::cout << "trax> message already seen" << *message << std::endl;
        }
    }
  }

  ~Subscriber() {
    std::lock_guard<std::mutex> lock_handler(_mutex_handler);
    _queue->unsubscribe(this);
  }
};

}  // namespace messages
}  // namespace neuro

#endif /* NEURO_SRC_MESSAGES_SUBSCRIBER_HPP */
