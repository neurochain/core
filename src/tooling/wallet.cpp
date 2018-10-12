#include "common/types.hpp"
#include "config.pb.h"
#include "consensus/PiiConsensus.hpp"
#include "crypto/Ecc.hpp"
#include "crypto/Hash.hpp"
#include "ledger/LedgerMongodb.hpp"
#include "messages.pb.h"
#include "messages/Hasher.hpp"
#include "messages/Message.hpp"
#include "tooling/genblock.hpp"

#include <sys/stat.h>
#include <boost/program_options.hpp>
#include <iostream>

namespace po = boost::program_options;
namespace neuro {

inline bool exists_file(const std::string &name) {
  struct stat buffer;
  return (stat(name.c_str(), &buffer) == 0);
}

class Wallet {
 private:
  std::unique_ptr<crypto::Ecc> _ecc;
  std::shared_ptr<ledger::Ledger> _ledger;
  std::vector<messages::Transaction> _trxs;

  std::unique_ptr<messages::Hasher> _address;

 public:
  Wallet(const std::string &pathpriv, const std::string &pathpub,
         std::shared_ptr<ledger::Ledger> ledger)
      : _ledger(ledger) {
    _ecc = std::make_unique<crypto::Ecc>(pathpriv, pathpub);

    Buffer key_pub_raw;
    _ecc->mutable_public_key()->save(&key_pub_raw);

    _address = std::make_unique<messages::Hasher>(key_pub_raw);
  }

  void getTransactions() {
    _trxs.clear();
    ledger::Ledger::Filter _filter;
    _filter.output_key_id(*_address);

    if (_ledger->for_each(_filter, [&](const messages::Transaction &a) {
          _trxs.push_back(std::move(a));
          return true;
        })) {
      std::cout << "Found " << _trxs.size() << " Trxs" << std::endl;
    } else {
      std::cout << "failed Query" << std::endl;
    }
  }

  void getLastBlockHeigth() {
    int32_t lastheight = _ledger->height();
    std::cout << "Height " << lastheight << std::endl;
  }

  void showlastBlock() {
    messages::Block b;
    if (_ledger->get_block(_ledger->height(), &b)) {
      std::string t;
      messages::to_json(b, &t);
      std::cout << t << std::endl;
    } else {
      std::cout << "Get Block " << _ledger->height() << " Failed" << std::endl;
    }
  }

  void testgenblock() {
    messages::Block block1;
    tooling::genblock::genblock_from_last_db_block(block1, _ledger, 1);

    Buffer buftmp;
    messages::to_buffer(block1, &buftmp);

    messages::Hasher testrand(buftmp);

    std::string t;
    messages::to_json(testrand, &t);

    std::cout << t << std::endl;

    _ledger->push_block(block1);
  }
  ~Wallet() {}
};

int main(int argc, char *argv[]) {
  po::options_description desc("Allowed options");
  desc.add_options()("help,h", "Produce help message.")(
      "keypub,p", po::value<std::string>()->default_value("key.pub"),
      "File path for public key (.pub)")(
      "key,k", po::value<std::string>()->default_value("key.priv"),
      "File path for private key (.priv)")(
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

  messages::config::Config _config;
  const auto configuration_filepath = vm["configuration"].as<std::string>();
  messages::from_json_file(configuration_filepath, &_config);

  const std::string keypubPath = vm["keypub"].as<std::string>();
  const std::string keyprivPath = vm["key"].as<std::string>();

  if (!exists_file(keypubPath) || !exists_file(keyprivPath)) {
    std::cout << "Pub || Priv Key not found" << std::endl;
    return 1;
  }

  auto db = _config.database();
  auto ledger = std::make_shared<ledger::LedgerMongodb>(db);

  // Wallet w(keyprivPath, keypubPath, _ledger);
  // w.getTransactions();
  // w.getLastBlockHeigth();
  // w.showlastBlock();
  // w.testgenblock();
  /*
      for(int i = 0; i < 10 ; i++){
          crypto::Ecc ecc({"../keys/key_" + std::to_string(i) + ".priv"
     },{"../keys/key_" + std::to_string(i) + ".pub"}); Buffer buf;
          ecc.public_key().save(&buf);

          messages::Address addr(buf);
          std::string t;
          messages::to_json(addr,&t);

          std::cout << i << " - " << t << std::endl;
      }*/

  ledger->fork_test();  ///!< Delete Fork branche

  consensus::PiiConsensus _PiiConsensus(ledger, 10);

  messages::Block block11;

  crypto::Ecc _ecc({"../keys/key_3.priv"}, {"../keys/key_3.pub"});
  Buffer key_pub_raw;
  _ecc.mutable_public_key()->save(&key_pub_raw);

  messages::KeyPub author;
  author.set_type(messages::KeyType::ECP256K1);
  author.set_raw_data(key_pub_raw.data(), key_pub_raw.size());

  tooling::genblock::genblock_from_last_db_block(
      block11, ledger, std::time(nullptr),
      std::make_optional<messages::KeyPub>(author), 9);

  _PiiConsensus.add_block(block11);

  return 0;
}
}  // namespace neuro
int main(int argc, char *argv[]) { return neuro::main(argc, argv); }
