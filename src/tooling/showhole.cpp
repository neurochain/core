#include <boost/program_options.hpp>
#include <iostream>
#include "config.pb.h"
#include "ledger/Ledger.hpp"
#include "ledger/LedgerMongodb.hpp"
#include "messages/Message.hpp"
#include "messages/config/Config.hpp"

namespace po = boost::program_options;
namespace neuro::tooling {

void check_blocks(const ledger::LedgerMongodb &ledger) {
  auto tagged_blocks = ledger.get_blocks(messages::MAIN);
  std::cerr << __FILE__ << ":" << __LINE__ << ":" << __FUNCTION__ << ": "
            << "checking block"
            << std::endl;
  int i = 0;
  for (auto tagged_block : tagged_blocks) {
    std::cerr << __FILE__ << ":" << __LINE__ << ":" << __FUNCTION__ << ": "
              << tagged_block.block().header().author().key_pub()
              << std::endl;
    i++;
  }
  std::cerr << __FILE__ << ":" << __LINE__ << ":" << __FUNCTION__ << ": "
            << "found " << i << " block"
            << std::endl;
}

int main(int argc, char *argv[]) {
  po::options_description desc("Allowed options");
  desc.add_options()("help,h", "Produce help message.")(
      "configuration,c", po::value<std::string>()->default_value("bot.json"));

  po::variables_map vm;
  po::store(po::parse_command_line(argc, argv, desc), vm);
  try {
    po::notify(vm);
  } catch (po::error &e) {
    return 1;
  }

  if (vm.count("help")) {
    std::cout << desc << std::endl;
    return 1;
  }

  if (!vm.count("configuration")) {
    std::cout << desc << std::endl;
    return 2;
  }

  const Path configuration_filepath = vm["configuration"].as<std::string>();
  messages::config::Config configuration(configuration_filepath);
  ledger::LedgerMongodb ledger(configuration.database());
  std::cerr << __FILE__ << ":" << __LINE__ << ":" << __FUNCTION__ << ": "
            << ledger.height() << std::endl;
  check_blocks(ledger);
  return 0;
}

}  // namespace neuro::tooling

int main(int argc, char *argv[]) { return neuro::tooling::main(argc, argv); }
