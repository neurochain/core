#include <gtest/gtest.h>
#include <typeinfo>

#include "messages.pb.h"
#include "src/messages/Message.hpp"
#include "src/messages/Queue.hpp"
#include "src/messages/Subscriber.hpp"

namespace neuro {
namespace messages {
namespace test {

void callback_hello1(const messages::Header &header,
                     const messages::Body &body) {}

void callback_hello2(const messages::Header &header,
                     const messages::Body &body) {}

void callback_world1(const messages::Header &header,
                     const messages::Body &body) {}

class QueueTest {
 public:
  void test_empty() {
    auto tested_queue = messages::Queue{};
    ASSERT_TRUE(tested_queue._queue.empty());
    ASSERT_TRUE(tested_queue.is_empty());
  }

  void test_subscribe() {
    auto tested_queue = messages::Queue{};
    messages::Subscriber sub_0(&tested_queue), sub_1(&tested_queue),
        sub_2(&tested_queue);
    sub_0.subscribe(messages::Type::kHello, callback_hello1);
    sub_1.subscribe(messages::Type::kHello, callback_hello2);
    sub_2.subscribe(messages::Type::kWorld, callback_world1);
    ASSERT_EQ(tested_queue._subscribers.size(), 3);
    std::size_t count_sub_0(0), count_sub_1(0), count_sub_2(0);
    for (const auto &sub : tested_queue._subscribers) {
      if (sub == &sub_0) {
        ++count_sub_0;
      } else if (sub == &sub_1) {
        ++count_sub_1;
      } else if (sub == &sub_2) {
        ++count_sub_2;
      } else {
        ASSERT_TRUE(sub == &sub_0 || sub == &sub_1 || sub == &sub_2);
      }
    }
    ASSERT_EQ(count_sub_0, 1);
    ASSERT_EQ(count_sub_1, 1);
    ASSERT_EQ(count_sub_2, 1);
  }

  void test_pushing_message() {
    auto tested_queue = messages::Queue{};
    tested_queue.push(std::make_shared<const messages::Message>());
    ASSERT_FALSE(tested_queue._queue.empty());
  };

  void test_message_broadcasting() {
    std::size_t count_hello(0);
    std::size_t count_world(0);
    auto tested_queue = messages::Queue{};
    tested_queue.run();
    messages::Subscriber sub_hello(&tested_queue);
    sub_hello.subscribe(
        messages::Type::kHello,
        [&count_hello](const messages::Header &, const messages::Body &) {
          ++count_hello;
        });
    messages::Subscriber sub_world(&tested_queue);
    sub_world.subscribe(
        messages::Type::kWorld,
        [&count_world](const messages::Header &, const messages::Body &) {
          ++count_world;
        });
    tested_queue.subscribe(&sub_hello);
    tested_queue.subscribe(&sub_world);
    auto message_hello = std::make_shared<messages::Message>();
    messages::Hello *hello = message_hello->add_bodies()->mutable_hello();
    ASSERT_NE(hello, nullptr);
    auto hello_kpub = message_hello->mutable_header()->mutable_key_pub();
    ASSERT_NE(hello_kpub, nullptr);
    hello_kpub->set_raw_data("CYxS/kJB4nHdBfBREEtqRL2xFGbi7Hg4rQkTxv9OwJs=");
    auto message_world = std::make_shared<messages::Message>();
    messages::World *world = message_world->add_bodies()->mutable_world();
    ASSERT_NE(world, nullptr);
    auto world_kpub = message_world->mutable_header()->mutable_key_pub();
    ASSERT_NE(world_kpub, nullptr);
    world_kpub->set_raw_data("CYxS/kJB4nHdBfBREEtqRL2xFGbi7Hg4rQkTxv9OwJs=");
    world->set_accepted(true);
    ASSERT_TRUE(tested_queue.push(std::move(message_hello)));
    ASSERT_TRUE(tested_queue.push(std::move(message_world)));
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    ASSERT_EQ(count_hello, 1);
    ASSERT_EQ(count_world, 1);
  }

  void test_unsubscribe() {
    std::size_t count_hello(0);
    std::size_t count_world(0);
    auto tested_queue = messages::Queue{};
    tested_queue.run();
    messages::Subscriber sub_hello(&tested_queue);
    sub_hello.subscribe(
        messages::Type::kHello,
        [&count_hello](const messages::Header &, const messages::Body &) {
          ++count_hello;
        });
    messages::Subscriber sub_world(&tested_queue);
    sub_world.subscribe(
        messages::Type::kWorld,
        [&count_world](const messages::Header &, const messages::Body &) {
          ++count_world;
        });
    auto message_hello = std::make_shared<messages::Message>();
    messages::Hello *hello = message_hello->add_bodies()->mutable_hello();
    ASSERT_NE(hello, nullptr);
    auto hello_kpub = message_hello->mutable_header()->mutable_key_pub();
    ASSERT_NE(hello_kpub, nullptr);
    hello_kpub->set_raw_data("CYxS/kJB4nHdBfBREEtqRL2xFGbi7Hg4rQkTxv9OwJs=");
    auto message_world = std::make_shared<messages::Message>();
    messages::World *world = message_world->add_bodies()->mutable_world();
    ASSERT_NE(world, nullptr);
    auto world_kpub = message_world->mutable_header()->mutable_key_pub();
    ASSERT_NE(world_kpub, nullptr);
    world_kpub->set_raw_data("CYxS/kJB4nHdBfBREEtqRL2xFGbi7Hg4rQkTxv9OwJs=");
    world->set_accepted(true);
    ASSERT_TRUE(tested_queue.push(std::move(message_hello)));
    ASSERT_TRUE(tested_queue.push(std::move(message_world)));
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    ASSERT_EQ(count_hello, 1);
    tested_queue.unsubscribe(&sub_world);
    ASSERT_EQ(count_world, 0);
  }
};

TEST(Queue, empty) {
  QueueTest mqt;
  mqt.test_empty();
}

TEST(Queue, subscribe) {
  QueueTest mqt;
  mqt.test_subscribe();
}

TEST(Queue, pushing_message) {
  QueueTest mqt;
  mqt.test_pushing_message();
}

TEST(Queue, message_broadcasting) {
  QueueTest mqt;
  mqt.test_message_broadcasting();
}

}  // namespace test
}  // namespace messages
}  // namespace neuro
