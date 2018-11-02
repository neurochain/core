#include "src/messages/Message.hpp"

#include <gtest/gtest.h>
#include <typeinfo>

namespace neuro {
namespace test {

TEST(Message, Message) {
  messages::Message message;
  ASSERT_FALSE(message.has_header());
  ASSERT_EQ(message.bodies_size(), 0);
}

TEST(Message, Hello) {
  messages::Message message;
  ASSERT_EQ(message.bodies_size(), 0);
  const messages::Hello* hello = message.add_bodies()->mutable_hello();
  ASSERT_NE(hello, nullptr);
  ASSERT_EQ(message.bodies_size(), 1);
  for (const auto& body : message.bodies()) {
    ASSERT_EQ(messages::get_type(body), messages::Type::kHello);
    ASSERT_FALSE(hello->has_key_pub());
    ASSERT_FALSE(hello->has_listen_port());
  }
}

TEST(Message, World) {
  messages::Message message;
  ASSERT_EQ(message.bodies_size(), 0);
  const messages::World* world = message.add_bodies()->mutable_world();
  ASSERT_NE(world, nullptr);
  ASSERT_EQ(message.bodies_size(), 1);
  for (const auto& body : message.bodies()) {
    ASSERT_EQ(messages::get_type(body), messages::Type::kWorld);
    ASSERT_FALSE(world->has_key_pub());
    ASSERT_FALSE(world->has_accepted());
    ASSERT_FALSE(world->accepted());
    ASSERT_EQ(world->peers_size(), 0);
  }
}

}  // namespace test
}  // namespace neuro
