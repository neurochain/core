#include <boost/program_options.hpp>
#include "crypto/Ecc.hpp"
#include "messages.pb.h"
#include "messages/Hasher.hpp"

namespace neuro {
namespace po = boost::program_options;

int main(int argc, char *argv[]) {
  po::options_description desc("Allowed options");
  desc.add_options()("help,h", "Produce help message.")(
      "private,k", po::value<std::string>()->default_value("key.priv"),
      "File path for private keys")(
      "public,p", po::value<std::string>()->default_value("key.pub"),
      "File path for private keys");

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

  crypto::Ecc ecc(vm["private"].as<std::string>(),
                  vm["public"].as<std::string>());
  std::cout << "public:  " << std::endl << ecc.public_key() << std::endl;
  std::cout << "private: " << std::endl << ecc.private_key() << std::endl;
  Buffer buf;
  ecc.public_key().save(&buf);
  messages::Hasher addr(buf);
  std::cout << "address : " << std::endl << addr << std::endl;

  return 0;
}

}  // namespace neuro

int main(int argc, char *argv[]) { return neuro::main(argc, argv); }
