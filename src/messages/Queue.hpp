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
#include "common/Queue.hpp"
#include "common/logger.hpp"
#include "messages/Message.hpp"

namespace neuro {
namespace messages {

namespace test {
class QueueTest;
}  // namespace test

class Subscriber;

class Queue :public ::neuro::Queue<Message, Subscriber> {

 public:
  Queue();
  friend class neuro::messages::test::QueueTest;
  
};
  
}  // namespace messages
}  // namespace neuro

#endif /* NEURO_SRC_MESSAGE_QUEUE_HPP */
