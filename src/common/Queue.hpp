#ifndef NEURO_SRC_COMMON_MESSAGEQUEUE_HPP
#define NEURO_SRC_COMMON_MESSAGEQUEUE_HPP

#include <array>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <queue>
#include <thread>
#include <unordered_set>
#include "common/logger.hpp"
#include "common/types.hpp"

namespace neuro {

/*!
   \Class Queue
         \brief Principal class used to handle Pub/Sub messages distribution in
   the network
 */

template <typename Payload, typename Subscriber>
class Queue {
 public:
  using Callback = std::function<void(std::shared_ptr<const Payload>)>;
  using Filter = std::function<bool(std::shared_ptr<const Payload>)>;

 protected:
  /*!
    \fn std::shared_ptr<const Payload>
          Queue::next_message() \brief Protected method to retrieve from the
          queue the next message to trait \return A shared pointer to a constant
          Payload
  */
  std::shared_ptr<const Payload> next_message() {
    std::lock_guard<std::mutex> lock_queue(_queue_mutex);
    auto message = _queue.front();
    _queue.pop();
    return message;
  }
  /*!
          \fn bool Queue::is_empty() const
          \brief Protected method to check if the message queue is empty or not
          \return True if there is no message in _queue, false otherwise
   */
  bool is_empty() {
    std::lock_guard<std::mutex> lock_queue(_queue_mutex);
    return _queue.empty();
  }
  /*!
                \fn bool Queue::do_work()
                \brief Protected method that has the main loop of the
     Queue

     It is called from the Queue::run() to enter the
     Queue thread and start its main loop
  */
  void do_work() {
    do {
      while (!_quitting && this->is_empty()) {  // avoid spurious wakeups
        std::unique_lock<std::mutex> lock_queue(_queue_mutex);
        _condition.wait_for(lock_queue, 1s);
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
          subscriber->handler(message);
        }
      }
    } while (!_quitting);
  }

 protected:
  bool _started{false};
  std::unordered_set<Subscriber *> _subscribers;
  /*!
    \var std::queue<std::shared_ptr<const Payload>> _queue
    \brief Main queue with all the Payload that has been pushed
     and need to be distributed
   */
  std::queue<std::shared_ptr<const Payload>> _queue;
  std::mutex _queue_mutex;
  mutable std::mutex _callbacks_mutex;
  std::atomic<bool> _quitting{false};
  std::thread _main_thread;
  std::condition_variable _condition;
  std::vector<Filter> _filters;

 public:
  Queue() = default;

  void add_filter_input(const Filter &filter) { _filters.push_back(filter); }

  bool push(std::shared_ptr<const Payload> message) {
    if (_quitting) {
      LOG_DEBUG << "Skip message because quitting";
      return false;
    }

    for (const auto &filter : _filters) {
      if (!filter(message)) {
        LOG_DEBUG << "Filtering out message";
        return false;
      }
    }

    {
      std::lock_guard<std::mutex> lock_queue(_queue_mutex);
      _queue.push(message);
    }
    _condition.notify_all();

    return true;
  }

  void subscribe(Subscriber *subscriber) {
    std::lock_guard<std::mutex> lock_callbacks(_callbacks_mutex);
    _subscribers.insert(subscriber);
  }

  void unsubscribe(Subscriber *subscriber) {
    std::lock_guard<std::mutex> lock_callbacks(_callbacks_mutex);
    _subscribers.erase(subscriber);
  }

  std::size_t size() const { return _queue.size(); }
  /*!
    \fn void Queue::run()
    \brief Public method that need to me called in order for the Queue to
    start processing the pushed messages for the subscribers.
  */
  void run() {
    _started = true;
    _main_thread = std::thread([this]() { this->do_work(); });
  }

  /*!
    \fn bool Queue::quit() const
    \brief Protected method to set the quitting flag.

    It is only call from the destructor and set the quitting flag so
    the Queue object can go out of the main loop and quit its thread.
  */
  void quit() {
    if (!_started) {
      return;
    }
    _quitting = true;
    _condition.notify_all();
    if (_main_thread.joinable()) {
      _main_thread.join();
    }
  }

  ~Queue() { quit(); }
};

}  // namespace neuro

#endif /* NEURO_SRC_COMMON_QUEUE_HPP */
