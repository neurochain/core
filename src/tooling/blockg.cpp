#include <boost/program_options.hpp>
#include "common/logger.hpp"
#include "common/types.hpp"
#include "crypto/Ecc.hpp"
#include "crypto/Hash.hpp"
#include "ledger/LedgerMongodb.hpp"
#include "messages.pb.h"
#include "messages/Hasher.hpp"
#include "messages/Message.hpp"

#include <fstream>
#include <iostream>

namespace po = boost::program_options;

namespace neuro {

inline bool exists_file(const std::string &name) {
  struct stat buffer;
  return (stat(name.c_str(), &buffer) == 0);
}

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
  transaction.mutable_fees()->set_value(0);  //"0");
                                             // return transaction;
}

void block0(uint32_t bots, const std::string &pathdir, messages::NCCSDF &nccsdf,
            const std::string &outfile, ledger::LedgerMongodb &ledger) {
  messages::Block b;
  messages::BlockHeader *header = b.mutable_header();

  crypto::Ecc ecc;
  Buffer key_pub_raw;
  ecc.mutable_public_key()->save(&key_pub_raw);

  messages::KeyPub *author = header->mutable_author();
  author->set_type(messages::KeyType::ECP256K1);
  author->set_raw_data(key_pub_raw.data(), key_pub_raw.size());

  header->mutable_timestamp()->set_data(time(0));

  auto previons_block_hash = header->mutable_previous_block_hash();
  previons_block_hash->set_data("");  ///!< load 0
  previons_block_hash->set_type(messages::Hash::Type::Hash_Type_SHA256);

  header->set_height(0);

  ///!< bots generateur
  for (uint32_t i = 0; i < bots; ++i) {
    // gen i keys
    crypto::Ecc ecc;
    ecc.save({pathdir + "/key_" + std::to_string(i) + ".priv"},
             {pathdir + "/key_" + std::to_string(i) + ".pub"});
    messages::Transaction *transaction = b.add_transactions();
    coinbase(ecc.public_key(), nccsdf, *transaction);
  }

  // const auto tmp = crypto::hash_sha3_256( b.SerializeAsString() );
  neuro::Buffer tmpbuffer(b.SerializeAsString());
  messages::Hasher hash_id(tmpbuffer);
  header->mutable_id()->CopyFrom(hash_id);
  ledger.push_block(b);

  std::ofstream blockfile0;
  blockfile0.open(outfile);
  blockfile0 << b.SerializeAsString();
  blockfile0.close();
}

int main(int argc, char *argv[]) {
  po::options_description desc("Allowed options");
  desc.add_options()("help,h", "Produce help message.")(
      "wallet,w", po::value<uint32_t>()->default_value(100),
      "Number of Wallet in BLock 0")(
      "keyspath,k", po::value<std::string>()->default_value("keys"),
      "File path for keys (appending .pub or .priv)")(
      "ncc,n", po::value<int64_t>()->default_value(1000),
      "How many ncc you want")(
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

  std::string jsonstr;
  messages::to_json(_config, &jsonstr);

  LOG_INFO << jsonstr;

  ///!< Log
  if (_config.has_logs()) {
    log::from_config(_config.logs());
  }

  LOG_INFO << "Load DB ...";

  if (!_config.has_database()) {
    // ledger_ = std::make_shared<ledger::LedgerMongodb>(_config.db().url(),
    LOG_INFO << "Database require";
    return 1;
  }

  std::string keypath = vm["keyspath"].as<std::string>();
  if (!exists_file(keypath)) {
    std::cout << "create dir : " << std::endl;
    std::cout << "mkdir " << keypath << std::endl;
    return 1;
  }

  LOG_INFO << "Load 1 ...";
  uint32_t bots = vm["wallet"].as<uint32_t>();
  int64_t ncc = vm["ncc"].as<int64_t>();

  LOG_INFO << "Load 1 ...";
  messages::NCCSDF nccsdf;
  nccsdf.set_value(ncc);  // std::to_string(ncc));
  auto db = _config.database();

  LOG_INFO << "Load 2 ...";
  ledger::LedgerMongodb ledger(_config.database().url(),
                               _config.database().db_name());
  LOG_INFO << "Load 3 ...";
  block0(bots, keypath, nccsdf, _config.database().block0_path(), ledger);

  return 0;
}

}  // namespace neuro

int main(int argc, char *argv[]) { return neuro::main(argc, argv); }
