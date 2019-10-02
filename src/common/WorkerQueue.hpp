#include "common/Queue.hpp"
#include "thread"

namespace neuro {

template <typename Payload>
class SimpleSubscriber {
  using Callback = std::function<void(const Payload &)>;

 private:
  Callback _callback;

 public:
  SimpleSubscriber(Callback callback) : _callback(callback) {}
  void handler(std::shared_ptr<const Payload> payload) { _callback(*payload); }
  virtual ~SimpleSubscriber() {}
};

template <typename Payload>
class WorkerQueue {
  using Callback = std::function<void(const Payload &)>;

 private:
  SimpleSubscriber<Payload> _subscriber;
  Queue<Payload, SimpleSubscriber<Payload>> _queue;
  std::thread _thread;

 public:
  WorkerQueue(Callback callback) : _subscriber(callback) {
    _queue.subscribe(&_subscriber);
    _thread = std::thread([this]() { this->_queue.run(); });
  }
  void enqueue(const Payload &payload) {
    _queue.publish(std::make_shared<const Payload>(payload));
  }
};

}  // namespace neuro
