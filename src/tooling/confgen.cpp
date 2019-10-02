#include <boost/program_options.hpp>

#include "messages/Message.hpp"
#include "messages/config/Config.hpp"

namespace neuro {
namespace po = boost::program_options;

int main(int argc, char *argv[]) {
  po::options_description desc("Allowed options");
  desc.add_options()("help,h", "Produce help message.")(
      "endpoints,e", po::value<std::vector<Path>>()->multitoken(), "Endpoints")(
      "conf,c", po::value<Path>()->required(), "Configuration file to update")(
      "port,p", po::value<uint16_t>()->default_value(1337), "Listenning port")(
      "output,o", po::value<Path>()->required(), "Output filepath");

  po::variables_map vm;
  po::store(po::parse_command_line(argc, argv, desc), vm);
  try {
    po::notify(vm);
  } catch (po::error &e) {
    std::cout << e.what() << std::endl;
    std::cout << desc << "\n";
    return 1;
  }
  if (vm.count("help") != 0u) {
    std::cout << "usage: " << desc << "\n";
    return 1;
  }

  const auto conf_path = vm["conf"].as<Path>();
  const auto endpoints = vm["endpoints"].as<std::vector<Path>>();
  const auto output = vm["output"].as<Path>();
  const auto port = vm["port"].as<uint16_t>();

  messages::config::Config config;
  if (!messages::from_json_file(conf_path.string(), &config)) {
    std::cout << "Could not load_from_point configuration" << std::endl;
    std::cout << desc << "\n";
    return 1;
  }

  for (const auto &endpoint : endpoints) {
    if (!filesystem::exists(endpoint)) {
      filesystem::create_directories(endpoint);
    }
    crypto::Ecc ecc;
    ecc.save(endpoint);
    auto peer = config.mutable_networking()->mutable_tcp()->add_peers();
    peer->set_endpoint(endpoint.string());
    peer->set_port(port);
    peer->mutable_key_pub()->CopyFrom(ecc.key_pub());
  }
  std::string s;
  messages::to_json(config, &s, true);

  std::ofstream out(output);
  out << s;
  out.close();

  return 0;
}

}  // namespace neuro

int main(int argc, char *argv[]) { return neuro::main(argc, argv); }
