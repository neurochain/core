#include <boost/program_options.hpp>
#include <fstream>
#include <iostream>
#include "common/logger.hpp"
#include "ledger/LedgerMongodb.hpp"
#include "messages/Message.hpp"

namespace po = boost::program_options;

namespace neuro {
namespace tooling {

int main(int argc, char *argv[]) {
  po::options_description description("Allowed options");
  description.add_options()("help,h", "Show help message")(
      "filename,f", po::value<std::string>()->default_value("data.0.testnet"))(
      "format", po::value<std::string>()->default_value("protobuf"),
      "block0 format: protobuf, bson, json");
  po::variables_map options;
  po::store(po::parse_command_line(argc, argv, description), options);
  const auto filename = options["filename"].as<std::string>();
  std::string format = options["format"].as<std::string>();
  try {
    po::notify(options);
  } catch (po::error &e) {
    return 1;
  }

  if (options.count("help")) {
    LOG_INFO << description;
    return 1;
  }

  std::ifstream file(filename);
  if (!file.is_open()) {
    LOG_ERROR << "Could not open file";
    return 1;
  }
  std::string str((std::istreambuf_iterator<char>(file)),
                  std::istreambuf_iterator<char>());
  messages::Block block0;
  if (format == "protobuf") {
    block0.ParseFromString(str);
  } else if (format == "bson") {
    ledger::bss::document document;
    document << str;
    messages::from_bson(document.view(), &block0);
  } else if (format == "json") {
    messages::from_json(str, &block0);
  } else {
    LOG_ERROR << "Unknown format";
    return 1;
  }
  LOG_INFO << block0;
  return 0;
}

}  // namespace tooling
}  // namespace neuro

int main(int argc, char *argv[]) { return neuro::tooling::main(argc, argv); }
