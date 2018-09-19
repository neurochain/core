#ifndef NEURO_SRC_MESSAGES_SUBSCRIBER_HPP
#define NEURO_SRC_MESSAGES_SUBSCRIBER_HPP

#include <typeinfo>
#include <typeindex>

namespace neuro {
namespace messages {

class Subscriber {
public:
  using Callback = std::function<void (const Header &header,
				       const Body &body)>;
  
private:
  std::shared_ptr<Queue> _queue;
  std::vector<std::optional<Callback>> _callbacks_by_type;

public:
  Subscriber(std::shared_ptr<Queue> queue) :
    _queue(queue),
    _callbacks_by_type(_NB_ELEMENTS)
  {}

  template <typename T>
  void subscribe(const Type type, Callback callback) {
    _queue->subscribe(this);
    _callbacks_by_type[type] = callback;
  }

  void handler(std::shared_ptr<const Message> message) {
    for (const auto &body : message->bodies()) {
      const auto type = get_type(body);
      auto opt = _callbacks_by_type[type];
      if (opt) {
	(*opt)(message->header(), body);
      }
    }
  }
  
  ~Subscriber() {
    _queue->unsubscribe(this);
  }
};


}  // messages
}  // neuro

#endif /* NEURO_SRC_MESSAGES_SUBSCRIBER_HPP */
