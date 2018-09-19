#include <google/protobuf/util/json_util.h>
#include <boost/program_options.hpp>
#include <chrono>
#include <iostream>
#include <string>
#include <fstream>
#include <streambuf>

#include "config.pb.h"
#include "mongo/mongo.hpp"
#include <google/protobuf/text_format.h>
#include <google/protobuf/message.h>

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
  
  std::ifstream t(configuration_filepath);
  std::string str((std::istreambuf_iterator<char>(t)),
		  std::istreambuf_iterator<char>());
  google::protobuf::util::JsonParseOptions options;
  auto r = google::protobuf::util::JsonStringToMessage(str, &config, options);

  std::string json_proto;
  std::string sout; 
  google::protobuf::TextFormat::PrintToString(config, &sout);
  google::protobuf::util::MessageToJsonString(config, &json_proto);
  
  auto doc = bsoncxx::from_json(str);
  auto mongo_json = bsoncxx::to_json(doc);

  std::cout << json_proto << std::endl;
  std::cout << "##################################################" << std::endl;
  std::cout << mongo_json << std::endl;

  
  return 0;
}
}  // namespace neuro

int main(int argc, char *argv[]) {
  //
  return neuro::main(argc, argv);
}
