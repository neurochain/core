#include <sys/stat.h>
#include <boost/program_options.hpp>

#include "common/logger.hpp"
#include "messages/Message.hpp"
#include "messages/NCCAmount.hpp"
#include "messages/config/Config.hpp"
#include "tooling/blockgen.hpp"

namespace po = boost::program_options;

namespace neuro {
namespace tooling {
namespace blockgen {

int main(int argc, char *argv[]) {
  po::options_description desc("Allowed options");
  desc.add_options()("help,h", "Produce help message.")(
      "nb-wallet,n", po::value<uint32_t>()->default_value(100),
      "Number of wallets in block 0")(
      "keyspath,k", po::value<Path>()->default_value("keys"),
      "File path for keys (appending .pub or .priv)")(
      "ncc,N", po::value<uint64_t>()->default_value(1152921504606846976lu),
      "How many ncc you want");

  po::variables_map vm;
  po::store(po::parse_command_line(argc, argv, desc), vm);
  try {
    po::notify(vm);
  } catch (po::error &e) {
    return 1;
  }

  if (vm.count("help") != 0u) {
    std::cout << desc << "\n";
    return 1;
  }

  const auto keypath = vm["keyspath"].as<Path>();
  if (!filesystem::exists(keypath)) {
    filesystem::create_directories(keypath);
  }

  uint32_t bots = vm["nb-wallet"].as<uint32_t>();
  uint64_t ncc = vm["ncc"].as<uint64_t>();

  messages::NCCAmount nccsdf;
  nccsdf.set_value(ncc);

  testnet_blockg(bots, keypath, nccsdf);

  return 0;
}

}  // namespace blockgen
}  // namespace tooling
}  // namespace neuro

int main(int argc, char *argv[]) {
  return neuro::tooling::blockgen::main(argc, argv);
}
