#include <google/protobuf/util/json_util.h>
#include <boost/program_options.hpp>
#include <chrono>
#include <fstream>
#include <iostream>
#include <streambuf>
#include <string>

#include "Bot.hpp"
#include "config.pb.h"
#include "messages/Message.hpp"
#include "messages/config/Config.hpp"
#include "version.h"

namespace neuro {

namespace po = boost::program_options;
using namespace std::chrono_literals;

int main(int argc, char *argv[]) {
  po::options_description desc("Allowed options");
  desc.add_options()("help,h", "Produce help message.")("version,v",
                                                        "Print version.")(
      "configuration,c", po::value<std::string>()->default_value("bot.json"),
      "Configuration path.")("port,p", po::value<Port>(),
                             "Override listenning port configuration")(
      "namespace,n", po::value<std::string>(),
      "Namespace used when running several bots on the same computer");

  po::variables_map vm;
  po::store(po::parse_command_line(argc, argv, desc), vm);
  try {
    po::notify(vm);
  } catch (po::error &e) {
    return 1;
  }

  if (vm.count("help")) {
    std::cout << desc << "\n";
    return 1;
  }

  if (vm.count("version")) {
    std::cout << GIT_COMMIT_HASH << "\n";
    return 0;
  }

  const Path configuration_filepath = vm["configuration"].as<std::string>();

  auto configuration = messages::config::Config{configuration_filepath};

  if (vm.count("port")) {
    const auto port = vm["port"].as<Port>();
    configuration.mutable_networking()->mutable_tcp()->set_port(port);
  }
  if (vm.count("namespace")) {
    const auto ns = vm["namespace"].as<std::string>();
    const auto original_dbname = configuration.database().db_name();
    const auto new_dbname = original_dbname + "_" + ns;
    configuration.mutable_database()->set_db_name(new_dbname);
  }

  Bot bot(configuration);
  bot.join();
  return 0;
}
}  // namespace neuro

int main(int argc, char *argv[]) {
  //
  return neuro::main(argc, argv);
}
