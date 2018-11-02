#include "Queue.hpp"

#include "messages/Hasher.hpp"
#include "messages/Message.hpp"
#include "messages/Subscriber.hpp"
namespace neuro {
namespace messages {

Queue::Queue() {}

bool Queue::expired(std::shared_ptr<const messages::Message> message) {
  const auto raw_ts = message->header().ts().data();
  return ((raw_ts - std::time(nullptr)) > MESSAGE_TTL);
}

bool Queue::publish(std::shared_ptr<const messages::Message> message) {
  if (_quitting) {
    LOG_DEBUG << "Skip message because quitting";
    return false;
  }

  if (expired(message)) {
    LOG_DEBUG << "Skip message because it's too old";
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
    while (!_quitting && this->is_empty()) {  // avoid spurious wakeups
      std::unique_lock<std::mutex> lock_queue(_queue_mutex);
      LOG_TRACE << "waiting";
      _condition.wait(lock_queue);
    }
    // we validate again that the woke up call was not because it is quitting
    if (_quitting) {
      break;
    }
    LOG_TRACE << "getting next message";
    auto message = this->next_message();
    // for every body in the message we get the type
    {
      std::lock_guard<std::mutex> lock_callbacks(_callbacks_mutex);
      LOG_TRACE << "sending to subscribers";
      for (auto &subscriber : _subscribers) {
        subscriber->handler(message);
      }
    }
  } while (!_quitting);
}

Queue::~Queue() { quit(); }

}  // namespace messages
}  // namespace neuro
