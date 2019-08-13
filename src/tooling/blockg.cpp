#include <bits/stdint-uintn.h>
#include <sys/stat.h>
#include <boost/preprocessor/seq/enum.hpp>
#include <boost/preprocessor/seq/size.hpp>
#include <boost/program_options.hpp>
#include <cstdint>

#include "common/logger.hpp"
#include "messages/Message.hpp"
#include "messages/NCCAmount.hpp"
#include "messages/config/Config.hpp"
#include "tooling/blockgen.hpp"

namespace po = boost::program_options;

namespace neuro {
namespace tooling {
namespace blockgen {

inline bool exists_file(const std::string &name) {
  struct stat buffer;
  return (stat(name.c_str(), &buffer) == 0);
}

int main(int argc, char *argv[]) {
  po::options_description desc("Allowed options");
  desc.add_options()("help,h", "Produce help message.")(
      "wallet,w", po::value<uint32_t>()->default_value(100),
      "Number of wallets in block 0")(
      "keyspath,k", po::value<std::string>()->default_value("keys"),
      "File path for keys (appending .pub or .priv)")(
      "ncc,n", po::value<uint64_t>()->default_value(1152921504606846976lu),
      "How many ncc you want")(
      "configuration,c", po::value<std::string>()->default_value("bot.json"),
      "Configuration path.");

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

  ///!< Load Config
  messages::config::Config _config;
  const auto configuration_filepath = vm["configuration"].as<std::string>();
  messages::from_json_file(configuration_filepath, &_config);

  std::string jsonstr;
  messages::to_json(_config, &jsonstr);

  LOG_INFO << jsonstr;

  ///!< Log
  if (_config.has_logs()) {
    log::from_config(_config.logs());
  }

  LOG_INFO << "Load DB ...";

  if (!_config.has_database()) {
    // ledger_ = std::make_shared<ledger::LedgerMongodb>(_config.db().url(),
    LOG_INFO << "Database require";
    return 1;
  }

  std::string keypath = vm["keyspath"].as<std::string>();
  if (!exists_file(keypath)) {
    std::cout << "create dir : " << std::endl;
    std::cout << "mkdir " << keypath << std::endl;
    return 1;
  }

  LOG_INFO << "Load 1 ...";
  uint32_t bots = vm["wallet"].as<uint32_t>();
  uint64_t ncc = vm["ncc"].as<uint64_t>();

  LOG_INFO << "Load 1 ...";
  messages::NCCAmount nccsdf;
  nccsdf.set_value(ncc);

  auto db = _config.database();
  /*block0(bots, keypath, nccsdf, ledger);*/
  testnet_blockg(bots, keypath, nccsdf);

  return 0;
}

}  // namespace blockgen
}  // namespace tooling
}  // namespace neuro

int main(int argc, char *argv[]) {
  return neuro::tooling::blockgen::main(argc, argv);
}
