#include "Queue.hpp"

#include "messages/Subscriber.hpp"

namespace neuro {
namespace messages {

Queue::Queue() {}

bool Queue::publish(std::shared_ptr<const messages::Message> message) {
  if (_quitting) {
    return false;
  }

  {
    std::lock_guard<std::mutex> lock_queue(_queue_mutex);
    _queue.push(message);
  }
  _condition.notify_all();

  return true;
}

void Queue::subscribe(Subscriber *subscriber) {
  std::lock_guard<std::mutex> lock_callbacks(_callbacks_mutex);
  _subscribers.insert(subscriber);
}

void Queue::unsubscribe(Subscriber *subscriber) {
  std::lock_guard<std::mutex> lock_callbacks(_callbacks_mutex);
  _subscribers.erase(subscriber);
}

void Queue::run() {
  _started = true;
  _main_thread = std::thread([this]() { this->do_work(); });
  _main_thread.detach();
}

bool Queue::is_empty() {
  std::lock_guard<std::mutex> lock_queue(_queue_mutex);
  return _queue.empty();
}

std::shared_ptr<const messages::Message> Queue::next_message() {
  std::lock_guard<std::mutex> lock_queue(_queue_mutex);
  auto message = _queue.front();
  _queue.pop();
  return message;
}

void Queue::quit() {
  if (!_started) {
    return;
  }
  _quitting = true;
  _condition.notify_all();
}

void Queue::do_work() {
  do {
    LOG_DEBUG << this << " After do in queue";
    while (!_quitting && this->is_empty()) { // avoid spurious wakeups
      std::unique_lock<std::mutex> lock_queue(_queue_mutex);
      _condition.wait(lock_queue);
    }
    // we validate again that the woke up call was not because it is quitting
    if (_quitting) {
      break;
    }
    auto message = this->next_message();
    // for every body in the message we get the type
    {
      std::lock_guard<std::mutex> lock_callbacks(_callbacks_mutex);
      for (auto &subscriber : _subscribers) {
        LOG_DEBUG << this << " Before calling " << subscriber;
        subscriber->handler(message);
      }
    }
  } while (!_quitting);
}

Queue::~Queue() { quit(); }

} // namespace messages
} // namespace neuro
