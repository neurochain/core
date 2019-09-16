#ifndef NEURO_SRC_TOOLING_CLI_HPP
#define NEURO_SRC_TOOLING_CLI_HPP

#include <Bot.hpp>

namespace neuro::tooling::cli {
class BotCli : public Bot {
private:
  std::string to_hash(const std::string &data) const {
    return "{data:\"" + data + "\"}";
  }

public:
  explicit BotCli(const messages::config::Config &c) : Bot(NoInit(), c) {
    _subscriber.subscribe(
        messages::Type::kHello,
        [&](const messages::Header &header, const messages::Body &body) {
          handler_hello(header, body);
        });

    _subscriber.subscribe(
        messages::Type::kWorld,
        [&](const messages::Header &header, const messages::Body &body) {
          handler_world(header, body);
        });

    _subscriber.subscribe(
        messages::Type::kConnectionClosed,
        [&](const messages::Header &header, const messages::Body &body) {
          handler_deconnection(header, body);
        });

    _subscriber.subscribe(
        messages::Type::kConnectionReady,
        [&](const messages::Header &header, const messages::Body &body) {
          handler_connection(header, body);
        });
  }

  void key(std::ostream &os) {
    os << _me;
  }

  void get_block(std::ostream &os, const std::string &id) const {
    messages::Message message;
    messages::fill_header(message.mutable_header());
    auto *get_block = message.add_bodies()->mutable_get_block();
    get_block->mutable_hash()->set_data(to_hash(id));
    get_block->set_count(1);
    send_one(message);
  }

  void connectOne(std::ostream &os) {
    keep_max_connections();
    for (auto it = _peers.begin(messages::Peer::CONNECTING); it != _peers.end(); ++it) {
      os << to_json(**it, true) << std::endl;
    }
  }
};
}  // namespace neuro::tooling
#endif // NEURO_SRC_TOOLING_CORE_CLI_HPP
