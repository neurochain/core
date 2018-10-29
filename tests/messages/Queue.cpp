#include <gtest/gtest.h>
#include <typeinfo>

#include "messages.pb.h"
#include "src/messages/Message.hpp"
#include "src/messages/Queue.hpp"
#include "src/messages/Subscriber.hpp"

namespace neuro
{
namespace test
{

template <typename T, typename... U>
size_t function_address(std::function<T(U...)> f)
{
  typedef T(fnType)(U...);
  fnType **fn_ptr = f.template target<fnType *>();
  return (size_t)*fn_ptr;
}

void callback_hello1(const messages::Header &header,
                     const messages::Body &body) {}

void callback_hello2(const messages::Header &header,
                     const messages::Body &body) {}

void callback_world1(const messages::Header &header,
                     const messages::Body &body) {}

class QueueTest
{
public:
  bool test_empty() {
    auto tested_queue = std::make_shared<messages::Queue>();
    if (!tested_queue->_queue.empty()) {
      return false;
    }
    return true;
  }

  bool test_subscribe()
  {
    auto tested_queue = std::make_shared<messages::Queue>();
    messages::Subscriber sub_0(tested_queue), sub_1(tested_queue), sub_2(tested_queue);
    sub_0.subscribe(messages::Type::kHello, callback_hello1);
    sub_1.subscribe(messages::Type::kHello, callback_hello2);
    sub_2.subscribe(messages::Type::kWorld, callback_world1);
    tested_queue->subscribe(&sub_0);
    tested_queue->subscribe(&sub_1);
    tested_queue->subscribe(&sub_2);
    if (tested_queue->_subscribers.size() != 3) {
      return false;
    }
    std::size_t count_sub_0(0), count_sub_1(0), count_sub_2(0);
    for (const auto &sub : tested_queue->_subscribers)
    {
      if (sub == &sub_0) {
        ++count_sub_0;
      } else if (sub == &sub_1) {
        ++count_sub_1;
      } else if (sub == &sub_2) {
        ++count_sub_2;
      } else {
        return false;
      }
    }
    if (count_sub_0 != 1 || count_sub_1 != 1 || count_sub_2 != 1)
    {
      return false;
    }
    return true;
  }

  bool test_pushing_message()
  {
    auto tested_queue = std::make_shared<messages::Queue>();
    tested_queue->publish(std::make_shared<const messages::Message>());
    if (tested_queue->_queue.empty()) {
      return false;
    }
    return true;
  };

  bool test_message_broadcasting()
  {
    std::size_t count_hello(0);
    std::size_t count_world(0);
    auto tested_queue = std::make_shared<messages::Queue>();
    tested_queue->run();
    messages::Subscriber sub_hello(tested_queue);
    sub_hello.subscribe(messages::Type::kHello,
     [&count_hello](const messages::Header &, const messages::Body &) { ++count_hello; });
    messages::Subscriber sub_world(tested_queue);
    sub_world.subscribe(messages::Type::kWorld,
     [&count_world](const messages::Header &, const messages::Body &) { ++count_world; });
    tested_queue->subscribe(&sub_hello);
    tested_queue->subscribe(&sub_world);
    auto message_hello = std::make_shared<messages::Message>();
    const messages::Hello* hello = message_hello->add_bodies()->mutable_hello();
    if (hello == nullptr) {
      return false;
    }
    auto message_world = std::make_shared<messages::Message>();
    const messages::World* world = message_world->add_bodies()->mutable_world();
    if (world == nullptr) {
      return false;
    }
    if (!tested_queue->publish(std::move(message_hello))) {
      return false;
    }
    if (!tested_queue->publish(std::move(message_world))) {
      return false;
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    return count_hello == 1 && count_world == 1;
  }
};

TEST(Queue, empty)
{
  QueueTest mqt;
  ASSERT_TRUE(mqt.test_empty());
}

TEST(Queue, subscribe)
{
  QueueTest mqt;
  ASSERT_TRUE(mqt.test_subscribe());
}

TEST(Queue, pushing_message)
{
  QueueTest mqt;
  ASSERT_TRUE(mqt.test_pushing_message());
}

TEST(Queue, message_broadcasting)
{
  QueueTest mqt;
  ASSERT_TRUE(mqt.test_message_broadcasting());
}

} // namespace test
} // namespace neuro
