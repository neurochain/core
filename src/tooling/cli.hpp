#ifndef NEURO_SRC_TOOLING_CLI_HPP
#define NEURO_SRC_TOOLING_CLI_HPP

#include <Bot.hpp>

namespace neuro::tooling::cli {
class BotCli : public Bot {
public:
  BotCli(messages::config::Config c) : Bot(c) {}
  void key(std::ostream &os) {
    os << _me << std::endl;
  }
};
}  // namespace neuro::tooling
#endif // NEURO_SRC_TOOLING_CORE_CLI_HPP
