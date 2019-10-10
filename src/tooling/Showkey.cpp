#include <boost/program_options.hpp>

#include "common/Buffer.hpp"
#include "crypto/Ecc.hpp"
#include "crypto/KeyPriv.hpp"
#include "crypto/KeyPub.hpp"
#include "messages/Address.hpp"
#include "messages/Message.hpp"

namespace neuro {
namespace po = boost::program_options;

int main(int argc, char *argv[]) {
  po::options_description desc("Allowed options");
  desc.add_options()("help,h", "Produce help message.")(
      "private,p", po::value<std::string>(), "File path for private key")(
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

  if (vm.count("private") != 1) {
    std::cout << desc << std::endl;
    return 2;
  }

  crypto::KeyPriv key_priv(std::make_shared<CryptoPP::AutoSeededRandomPool>(),
                           vm["private"].as<std::string>());
  auto key_pub = key_priv.make_key_pub();
  if (vm["buffer"].as<bool>()) {
    Buffer pub_buffer;
    key_pub.save(&pub_buffer);
    std::cout << "public buffer:  " << std::endl << pub_buffer << std::endl;

    Buffer priv_buffer;
    key_priv.save(&priv_buffer);
    std::cout << "private buffer:  " << std::endl << priv_buffer << std::endl;
  } else {
    std::cout << "public:\n" << key_pub << std::endl;
    std::cout << "private:\n" << key_priv << std::endl;
    std::cout << "private exponent:\n"
              << std::hex << key_priv.exponent() << std::endl;
  }

  return 0;
}

}  // namespace neuro

int main(int argc, char *argv[]) { return neuro::main(argc, argv); }
