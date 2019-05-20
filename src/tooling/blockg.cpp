#include <boost/program_options.hpp>
#include "common/logger.hpp"
#include "common/types.hpp"
#include "crypto/Ecc.hpp"
#include "crypto/Hash.hpp"
#include "ledger/LedgerMongodb.hpp"
#include "messages.pb.h"
#include "messages/Hasher.hpp"
#include "messages/Message.hpp"
#include "messages/config/Config.hpp"
#include "tooling/blockgen.hpp"

#include <fstream>
#include <iostream>

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
      "Directory where keys will be stored")(
      "ncc,n", po::value<uint64_t>()->default_value(1152921504606846976lu),
      "How ncc you want?");

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

  std::string keypath = vm["keyspath"].as<std::string>();
  if (!exists_file(keypath)) {
    std::cout << "create dir : " << std::endl;
    std::cout << "mkdir " << keypath << std::endl;
    return 1;
  }

  uint32_t bots = vm["wallet"].as<uint32_t>();
  uint64_t ncc = vm["ncc"].as<uint64_t>();

  LOG_INFO << "Load 1 ...";
  messages::NCCAmount nccsdf;
  nccsdf.set_value(ncc);

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
