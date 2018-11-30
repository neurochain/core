#include <gtest/gtest.h>

#include "messages/Subscriber.hpp"
#include "src/messages/Queue.hpp"

namespace neuro {
namespace test {

namespace {

std::shared_ptr<messages::Message> getWorldMessage() {
  auto message_world = std::make_shared<messages::Message>();
  messages::World *world = message_world->add_bodies()->mutable_world();
  auto world_kpub = world->mutable_key_pub();
  world_kpub->set_type(messages::ECP256K1);
  world_kpub->set_raw_data(
      "MIIBMzCB7AYHKoZIzj0CATCB4AIBATAsBgcqhkjOPQEBAiEA//////////////////////"
      "///////////////v///"
      "C8wRAQgAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAEIAAAAAAAAAAAAAAAAAA"
      "AAAAAAAAAAAAAAAAAAAAAAAAHBEEEeb5mfvncu6xVoGKVzocLBwKb/"
      "NstzijZWfKBWxb4F5hIOtp3JqPEZV2k+/wOEQio/Re0SKaFVBmcR9CP+xDUuAIhAP/////"
      "///////////////66rtzmr0igO7/SXozQNkFBAgEBA0IABOBPdJmNMRu7dZ0O4+b/"
      "jG5CyuLeI870VKYu0DrtJ8I8VW3wt5NcbqfqIk7OI0+9cE7+xCPtKwF1vAHi730nMJ0=");
  world->set_accepted(true);
  return message_world;
}

}  // namespace

TEST(Subscriber, handler) {
  auto queue = std::make_shared<messages::Queue>();
  messages::Subscriber tested_sub(queue);
  auto message = getWorldMessage();
  tested_sub.handler(message);
}

TEST(Subscriber, is_new_body) {
  auto queue = std::make_shared<messages::Queue>();
  messages::Subscriber tested_sub(queue);
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
