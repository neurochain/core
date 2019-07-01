#include <gtest/gtest.h>

#include "messages/Subscriber.hpp"
#include "src/messages/Queue.hpp"

namespace neuro {
namespace test {

namespace {

std::shared_ptr<messages::Message> getWorldMessage() {
  auto message_world = std::make_shared<messages::Message>();
  auto world = message_world->add_bodies()->mutable_world();
  world->set_accepted(true);
  return message_world;
}

}  // namespace

TEST(Subscriber, handler) {
  auto queue = messages::Queue{};
  messages::Subscriber tested_sub(&queue);
  auto message = getWorldMessage();
  tested_sub.handler(message);
}

TEST(Subscriber, is_new_body) {
  auto queue = messages::Queue{};
  messages::Subscriber tested_sub(&queue);
  auto message = getWorldMessage();
  for (const auto &body : message->bodies()) {
    ASSERT_TRUE(tested_sub.is_new_body(std::time_t(nullptr), body));
  }
  tested_sub.handler(message);
  for (const auto &body : message->bodies()) {
    ASSERT_FALSE(tested_sub.is_new_body(std::time_t(nullptr), body));
  }
  tested_sub.handler(message);
}

}  // namespace test
}  // namespace neuro
