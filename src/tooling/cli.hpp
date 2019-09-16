#ifndef NEURO_SRC_TOOLING_CLI_HPP
#define NEURO_SRC_TOOLING_CLI_HPP

#include <Bot.hpp>

namespace neuro::tooling::cli {
class BotCli : public Bot {
private:
  std::string to_hash(std::string data) const {
    return "{data:\"" + data + "\"}";
  }
public:
  explicit BotCli(messages::config::Config c) : Bot(c) {}
  void key(std::ostream &os) {
    os << _me;
  }

  void get_block(std::string id) const {
    messages::Message message;
    messages::fill_header(message.mutable_header());
    auto *get_block = message.add_bodies()->mutable_get_block();
    get_block->mutable_hash()->set_data(to_hash(id));
    get_block->set_count(1);
    send_one(message);
  }
};
}  // namespace neuro::tooling
#endif // NEURO_SRC_TOOLING_CORE_CLI_HPP
