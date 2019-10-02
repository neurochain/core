#include "Queue.hpp"
#include "common/logger.hpp"
#include "messages/Queue.hpp"
#include "messages/Subscriber.hpp"

namespace neuro {
namespace messages {

Queue::Queue() {
  add_filter_input([] (std::shared_ptr<const messages::Message> message) ->bool {
                     const auto raw_ts = message->header().ts().data();
                     return (std::abs(std::time(nullptr) - raw_ts) <= MESSAGE_TTL);
                   });
}

}  // namespace messages
}  // namespace neuro
