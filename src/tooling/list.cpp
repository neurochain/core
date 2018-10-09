#include <boost/program_options.hpp>
#include "common/types.hpp"
#include "crypto/Ecc.hpp"
#include "crypto/Hash.hpp"
#include "messages.pb.h"
#include "messages/Hasher.hpp"
#include "messages/Message.hpp"

namespace po = boost::program_options;

namespace neuro {

int main(int argc, char *argv[]) {
  po::options_description desc("Allowed options");
  desc.add_options()("help,h", "Produce help message.")(
      "key,k", po::value<std::string>()->default_value("key.pub"),
      "File path to public key");

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
  const auto type = vm["type"].as<std::string>();
  const auto ncc = vm["ncc"].as<uint64_t>();

  crypto::EccPub ecc_pub(filepath);
  messages::Hasher address(ecc_pub);

  messages::Transaction transaction;
  auto input = transaction.add_inputs();

  auto input_id = input->mutable_id();
  input_id->set_data("");
  input->set_output_id(0);

  transaction.add_outputs()->mutable_address()->CopyFrom(address);

  // transaction.set_fees(ncc);
  transaction.mutable_fees()->set_value(std::to_string(ncc));

  if (type == "json") {
    std::string t;
    messages::to_json(transaction, &t);
    std::cout << t << std::endl;
  } else if (type == "bson") {
    std::cout << "bson output not implemented" << std::endl;
  } else if (type == "protobuf") {
    Buffer t;
    messages::to_buffer(transaction, &t);
    std::cout << t << std::endl;
  } else {
    std::cout << "Wrong output type" << std::endl;
    std::cout << desc << "\n";
    return 1;
  }

  return 0;
}

}  // namespace neuro

int main(int argc, char *argv[]) { return neuro::main(argc, argv); }
