#include <boost/program_options.hpp>
#include "common/logger.hpp"
#include "common/types.hpp"
#include "crypto/Ecc.hpp"
#include "crypto/Hash.hpp"
#include "ledger/LedgerMongodb.hpp"
#include "messages.pb.h"
#include "messages/Hasher.hpp"
#include "messages/Message.hpp"

namespace po = boost::program_options;

namespace neuro {

void coinbase(const crypto::EccPub &key_pub, const messages::NCCSDF &ncc,
              messages::Transaction &transaction) {
  // messages::Transaction transaction;
  Buffer key_pub_raw;
  key_pub.save(&key_pub_raw);
  messages::Hasher address(key_pub_raw);

  auto input = transaction.add_inputs();

  auto input_id = input->mutable_id();
  input_id->set_type(messages::Hash::SHA256);
  input_id->set_data("");
  input->set_output_id(0);
  input->set_key_id(0);

  auto output = transaction.add_outputs();
  output->mutable_address()->CopyFrom(address);
  output->mutable_value()->CopyFrom(ncc);
  transaction.mutable_fees()->set_value("0");

  // return transaction;
}

int main(int argc, char *argv[]) {
  po::options_description desc("Allowed options");
  desc.add_options()("help,h", "Produce help message.")(
      "keypath,k", po::value<std::string>()->default_value("keys"),
      "File path for keys (appending .pub or .priv)")(
      "ncc,n", po::value<uint64_t>()->default_value(1000),
      "How many ncc you want")("type,t",
                               po::value<std::string>()->default_value("json"),
                               "enum [json, bson, protobuf]")(
      "configuration,c", po::value<std::string>()->default_value("bot.json"),
      "Configuration path.");

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

  ///!< Load Config
  messages::config::Config _config;
  const auto configuration_filepath = vm["configuration"].as<std::string>();
  messages::from_json_file(configuration_filepath, &_config);

  ///!< Log
  if (_config.has_logs()) {
    log::from_config(_config.logs());
  }

  LOG_INFO << "Load DB ...";
  /*
  if (_config.has_db()){
     //ledger_ = std::make_shared<ledger::LedgerMongodb>(_config.db().url(),
  _config.db().db_name());* LOG_INFO << _config.db().url() ;
  }*/

  const auto keypath = vm["keypath"].as<std::string>();
  const auto type = vm["type"].as<std::string>();
  const auto ncc = vm["ncc"].as<uint64_t>();

  messages::NCCSDF nccsdf;
  nccsdf.set_value(std::to_string(ncc));
  crypto::EccPub key_pub(keypath);

  messages::Block b;
  messages::BlockHeader *header = b.mutable_header();

  Buffer key_pub_raw;
  key_pub.save(&key_pub_raw);

  messages::KeyPub *author = header->mutable_author();
  author->set_type(messages::KeyType::ECP256K1);
  author->set_raw_data(key_pub_raw.data(), key_pub_raw.size());

  header->mutable_timestamp()->set_data(time(0));

  auto previons_block_hash = header->mutable_previous_block_hash();
  previons_block_hash->set_data("");  ///!< load 0
  previons_block_hash->set_type(messages::Hash::Type::Hash_Type_SHA256);

  header->set_height(0);

  messages::Transaction *transaction = b.add_transactions();
  coinbase(key_pub, nccsdf, *transaction);
  header->mutable_id()->set_data("");

  // const auto tmp = crypto::hash_sha3_256( b.SerializeAsString() );
  neuro::Buffer tmpbuffer(b.SerializeAsString());
  messages::Hasher hash_id(tmpbuffer);
  header->mutable_id()->CopyFrom(hash_id);

  std::string t;
  messages::to_json(b, &t);
  LOG_INFO << t;

  ledger::LedgerMongodb ledger_("mongodb://127.0.0.1:27017/neuro", "neuro");
  ledger_.push_block(b);

  /*
  if(type == "json") {
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
  }*/

  return 0;
}

}  // namespace neuro

int main(int argc, char *argv[]) { return neuro::main(argc, argv); }
