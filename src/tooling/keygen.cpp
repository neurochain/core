#include <boost/program_options.hpp>
#include "crypto/Ecc.hpp"
#include "messages.pb.h"

namespace po = boost::program_options;

namespace neuro {

int main(int argc, char *argv[]) {
  po::options_description desc("Allowed options");
  desc.add_options()("help,h", "Produce help message.")(
      "filepath,f", po::value<std::string>()->default_value("key"),
      "File path for keys (appending .pub or .priv)");

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

  const auto filepath = vm["filepath"].as<std::string>();

  crypto::Ecc ecc;
  const auto res = ecc.save({filepath + ".priv"}, {filepath + ".pub"});
  std::cout << "public:  " << ecc.public_key() << std::endl;
  std::cout << "private: " << ecc.private_key() << std::endl;

  return ((res) ? 0 : 1);
}

}  // namespace neuro

int main(int argc, char *argv[]) { return neuro::main(argc, argv); }
