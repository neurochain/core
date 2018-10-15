#ifndef NEURO_SRC_MESSAGES_MESSAGEQUEUE_HPP
#define NEURO_SRC_MESSAGES_MESSAGEQUEUE_HPP

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
#include "messages/Message.hpp"

namespace neuro {
namespace messages {

class Subscriber;

namespace test {
class QueueTest;
}

/*!
   \Class Queue
         \brief Principal class used to handle Pub/Sub messages distribution in
   the network
 */
class Queue {
 public:
  using Callback =
      std::function<void(std::shared_ptr<const messages::Message>)>;

 protected:
  /*!
          \fn std::shared_ptr<const messages::Message>
     Queue::next_message() \brief Protected method to retrieve from the
     queue the next message to trait \return A shared pointer to a constant
     messages::Message
   */
  std::shared_ptr<const messages::Message> next_message();
  /*!
          \fn bool Queue::is_empty() const
          \brief Protected method to check if the message queue is empty or not
          \return True if there is no message in _queue, false otherwise
   */
  bool is_empty();
  /*!
                \fn bool Queue::do_work()
                \brief Protected method that has the main loop of the
     Queue

                It is called from the Queue::run() to enter the
     Queue thread and start its main loop
         */
  void do_work();

 private:
  bool _started{false};
  std::unordered_set<Subscriber *> _subscribers;
  /*!
    \var std::queue<std::shared_ptr<const messages::Message>> _queue
    \brief Main queue with all the messages::Message that has been pushed
     and need to be distributed
   */
  std::queue<std::shared_ptr<const messages::Message>> _queue;
  std::mutex _queue_mutex;
  mutable std::mutex _callbacks_mutex;
  std::atomic<bool> _quitting{false};
  std::thread _main_thread;
  std::condition_variable _condition;

  std::unordered_set<std::shared_ptr<Buffer>> _seen_messages_hash;
  std::map<std::time_t, std::shared_ptr<Buffer>> _message_hash_by_ts;

  // bool clear_history();
  bool is_new_messages(std::shared_ptr<const messages::Message> message);

 public:
  Queue();
  ~Queue();

  bool publish(std::shared_ptr<const messages::Message>);
  void subscribe(Subscriber *subscriber);
  void unsubscribe(Subscriber *subscriber);

  /*!
    \fn void Queue::run()
    \brief Public method that need to me called in order for the Queue to
    start processing the pushed messages for the subscribers.
  */
  void run();

  /*!
    \fn bool Queue::quit() const
    \brief Protected method to set the quitting flag.

    It is only call from the destructor and set the quitting flag so
    the Queue object can go out of the main loop and quit its thread.
  */
  void quit();

  friend class neuro::messages::test::QueueTest;
};

}  // namespace messages
}  // namespace neuro

#endif /* NEURO_SRC_MESSAGE_QUEUE_HPP */
