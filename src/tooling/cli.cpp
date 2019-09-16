#include <cli/cli.h>
#include <cli/clifilesession.h>
#include <functional>

#include "tooling/cli.hpp"

namespace neuro::tooling::cli {

using namespace ::cli;
namespace po = boost::program_options;
int main(int argc, char* argv[]) {
  po::options_description desc("Launch a bot and open a cli to manage it");
  desc.add_options()("configuration,c", po::value<std::string>());
  po::variables_map vm;
  po::store(po::parse_command_line(argc, argv, desc), vm);
  try {
    po::notify(vm);
  } catch (po::error &e) {
    return 1;
  }

  if (vm.count("configuration") == 0) {
    std::cout << desc << "\n";
    return 2;
  }

  const Path configuration_filepath = vm["configuration"].as<std::string>();
  auto configuration = messages::config::Config{configuration_filepath};
  BotCli bot(configuration);

  auto menu = std::make_unique<Menu>("bot-cli");
  menu->Insert(
      "key",
      [&](std::ostream &os) { bot.key(os); },
      "display public key");

  Cli bot_cli(std::move(menu));
  SetColor();
  CliFileSession input(bot_cli);
  input.Start();
  return 0;
}

}  // namespace neuro::tooling::cli

int main(int argc, char* argv[]) {
  return neuro::tooling::cli::main(argc, argv);
}
