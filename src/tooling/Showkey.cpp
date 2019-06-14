#include <boost/program_options.hpp>
#include "crypto/Ecc.hpp"
#include "messages.pb.h"
#include "messages/Address.hpp"
#include "messages/Message.hpp"

namespace neuro {
namespace po = boost::program_options;

int main(int argc, char *argv[]) {
  po::options_description desc("Allowed options");
  desc.add_options()("help,h", "Produce help message.")(
      "private,k", po::value<std::string>(),
      "File path for private keys")(
      "public,p", po::value<std::string>(),
      "File path for private keys")(
      "buffer,b", po::bool_switch()->default_value(false),
      "display key as hex value");

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

  if (vm.count("private") + vm.count("public") != 2) {
    std::cout << desc << std::endl;
    return 2;
  }

  crypto::Ecc ecc(vm["private"].as<std::string>(),
                  vm["public"].as<std::string>());
  if (vm["buffer"].as<bool>()) {
    Buffer pub_buffer;
    ecc.key_pub().save(&pub_buffer);
    std::cout << "public buffer:  " << std::endl << pub_buffer << std::endl;

    Buffer priv_buffer;
    ecc.key_priv().save(&priv_buffer);
    std::cout << "private buffer:  " << std::endl << priv_buffer << std::endl;
  } else {
    std::cout << "public:\n" << ecc.key_pub() << std::endl;
    std::cout << "private:\n" << ecc.key_priv() << std::endl;
    std::cout << "private exponent:\n" << std::hex << ecc.key_priv().exponent() << std::endl;
  }

  messages::Address addr(ecc.key_pub());
  std::cout << "address : " << std::endl << addr << std::endl;

  return 0;
}

}  // namespace neuro

int main(int argc, char *argv[]) { return neuro::main(argc, argv); }
