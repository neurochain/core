#include <boost/program_options.hpp>
#include <boost/program_options/value_semantic.hpp>
#include <boost/program_options/variables_map.hpp>

#include "common/Buffer.hpp"
#include "crypto/KeyPub.hpp"
#include "messages.pb.h"
#include "messages/Message.hpp"
#include "messages/NCCAmount.hpp"

namespace po = boost::program_options;

namespace neuro {

messages::Transaction coinbase(const crypto::KeyPub &key_pub,
                               const messages::NCCAmount &ncc) {
  messages::Transaction transaction;
  Buffer key_pub_raw;
  key_pub.save(&key_pub_raw);

  auto output = transaction.add_outputs();
  output->mutable_key_pub()->CopyFrom(key_pub);
  output->mutable_value()->CopyFrom(ncc);
  transaction.mutable_fees()->set_value(0);  //"0");

  return transaction;
}

int main(int argc, char *argv[]) {
  po::options_description desc("Allowed options");
  desc.add_options()("help,h", "Produce help message.")(
      "keypath,k", po::value<std::string>()->default_value("keys"),
      "File path for keys (appending .pub or .priv)")(
      "ncc,n", po::value<uint64_t>()->default_value(1000),
      "How many ncc you want")("type,t",
                               po::value<std::string>()->default_value("json"),
                               "enum [json, bson, protobuf]");

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

  const auto keypath = vm["keypath"].as<std::string>();
  const auto type = vm["type"].as<std::string>();
  const auto ncc = vm["ncc"].as<uint64_t>();

  messages::NCCAmount nccsdf;
  nccsdf.set_value(ncc);  // std::to_string(ncc));
  crypto::KeyPub key_pub(keypath);
  messages::Transaction transaction = coinbase(key_pub, nccsdf);

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
