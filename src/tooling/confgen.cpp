#include <boost/program_options.hpp>
#include <fstream>
#include <vector>

#include "crypto/Ecc.hpp"
#include "messages.pb.h"
#include "messages/Message.hpp"
#include "messages/config/Config.hpp"
#include "networking/PeerPool.hpp"

namespace neuro {
namespace po = boost::program_options;

int main(int argc, char *argv[]) {
  po::options_description desc("Allowed options");
  desc.add_options()("help,h", "Produce help message.")(
      "keys,k", po::value<std::vector<std::string>>()->multitoken(),
      "Key file to include")(
      "endpoint,e", po::value<std::vector<std::string>>()->multitoken(),
      "Endpoint in same order as keys")("conf,c",
                                        po::value<std::string>()->required(),
                                        "Configuration file to update")(
      "output,o", po::value<std::string>()->required(), "Output filepath");

  po::variables_map vm;
  po::store(po::parse_command_line(argc, argv, desc), vm);
  try {
    po::notify(vm);
  } catch (po::error &e) {
    std::cout << e.what() << std::endl;
    std::cout << desc << "\n";
    return 1;
  }
  if (vm.count("help")) {
    std::cout << "usage: " << desc << "\n";
    return 1;
  }

  const auto conf_path = vm["conf"].as<std::string>();
  const auto keys_path = vm["keys"].as<std::vector<std::string>>();
  const auto endpoints = vm["endpoint"].as<std::vector<std::string>>();
  const auto output = vm["output"].as<std::string>();

  messages::config::Config config;
  if (!messages::from_json_file(conf_path, &config)) {
    std::cout << "Could not load configuration" << std::endl;
    std::cout << desc << "\n";
    return 1;
  }

  if (keys_path.size() != endpoints.size()) {
    std::cout << "keys and endpoints should have the same number of elements";
    std::cout << desc << "\n";
    return 1;
  }

  auto itKey = keys_path.cbegin();
  auto endKey = keys_path.cend();
  auto itEndpoint = endpoints.cbegin();

  networking::PeerPool peer_pool(config.networking().tcp().peers());
  messages::Peers peers;
  while (itKey != endKey) {
    messages::KeyPub key_pub;
    crypto::EccPub ecc_pub(*itKey);
    ecc_pub.save(&key_pub);
    auto peer = peers.add_peers();
    peer->set_endpoint(*itEndpoint);
    peer->set_port(1337);
    peer->mutable_key_pub()->CopyFrom(key_pub);

    ++itKey;
    ++itEndpoint;
  }
  peer_pool.insert(peers);
  std::string s;
  messages::to_json(config, &s);

  std::ofstream out(output);
  out << s;
  out.close();

  return 0;
}

}  // namespace neuro

int main(int argc, char *argv[]) { return neuro::main(argc, argv); }
