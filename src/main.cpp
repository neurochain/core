#include <google/protobuf/util/json_util.h>
#include <boost/program_options.hpp>
#include <chrono>
#include <iostream>
#include <string>
#include <fstream>
#include <streambuf>

#include "config.pb.h"
#include "mongo/mongo.hpp"
#include "messages/Message.hpp"

namespace neuro {

namespace po = boost::program_options;
using namespace std::chrono_literals;
  

int main(int argc, char *argv[]) {
  po::options_description desc("Allowed options");
  desc.add_options()
    ("help,h", "Produce help message.")
    ("configuration,c", po::value<std::string>()->default_value("bot.json"), "Configuration path.")
    ;

  po::variables_map vm;
  po::store(po::parse_command_line(argc, argv, desc), vm);
  try {
    po::notify(vm);
  } catch (po::error &e) {
    return 1;
  }

  if (vm.count("help")) {
    return 1;
  }

  const auto configuration_filepath = vm["configuration"].as<std::string>();
  messages::config::Config config;
  messages::from_json_file(configuration_filepath, &config);
  
  
  
  return 0;
}
}  // namespace neuro

int main(int argc, char *argv[]) {
  //
  return neuro::main(argc, argv);
}
