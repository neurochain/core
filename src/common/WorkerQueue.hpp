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
class WorkerQueue : public Queue<Payload, SimpleSubscriber<Payload>> {
  using Callback = std::function<void(const Payload &)>;

 private:
  SimpleSubscriber<Payload> _subscriber;

 public:
  WorkerQueue(Callback callback) : _subscriber(callback) {
    this->subscribe(&_subscriber);
    this->run();
  }

  ~WorkerQueue() { this->unsubscribe(&_subscriber); }
};

}  // namespace neuro
