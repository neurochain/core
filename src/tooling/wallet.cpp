#include "common/types.hpp"
#include "config.pb.h"
#include "consensus/PiiConsensus.hpp"
#include "crypto/Ecc.hpp"
#include "crypto/Hash.hpp"
#include "ledger/LedgerMongodb.hpp"
#include "messages.pb.h"
#include "messages/Hasher.hpp"
#include "messages/Message.hpp"
#include "messages/config/Config.hpp"
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
    _filter.output_address(*_address);

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

  ~Wallet() {}
};

messages::KeyPub load_key_pub(int i) {
  crypto::Ecc _ecc({"../keys/key_" + std::to_string(i) + ".priv"},
                   {"../keys/key_" + std::to_string(i) + ".pub"});
  Buffer key_pub_raw;
  _ecc.mutable_public_key()->save(&key_pub_raw);

  messages::KeyPub author;
  author.set_type(messages::KeyType::ECP256K1);
  author.set_raw_data(key_pub_raw.data(), key_pub_raw.size());
  return author;
}

void load_id_transaction(neuro::messages::Block *block) {
  for (int i = 0; i < block->transactions_size(); ++i) {
    neuro::messages::Transaction *transaction = block->mutable_transactions(i);
    if (!transaction->has_id()) {
      neuro::messages::Transaction _transaction(*transaction);
      _transaction.clear_id();

      Buffer buf;
      messages::to_buffer(_transaction, &buf);
      messages::Hasher newit(buf);
      transaction->mutable_id()->CopyFrom(newit);
    }

    if (!transaction->has_block_id()) {
      transaction->mutable_block_id()->CopyFrom(block->header().id());
    }
  }
}

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
  //  /*
  //        for(int i = 0; i < 1 ; i++){
  //            crypto::Ecc ecc({"../keys/key_" + std::to_string(i) + ".priv"
  //       },{"../keys/key_" + std::to_string(i) + ".pub"});
  //
  //            Buffer buf;
  //            ecc.public_key().save(&buf);
  //
  //            messages::Address addr(buf);
  //            std::string t;
  //            messages::to_json(addr,&t);
  //
  //            std::cout << i << " - " << t << std::endl;
  //
  //            messages::KeyPub keypub;
  //            keypub.set_type(messages::KeyType::ECP256K1);
  //            keypub.set_raw_data(buf.data(),buf.size());
  //
  //            t = "";
  //            messages::to_json(keypub,&t);
  //            std::cout << i << " - " << t << std::endl;
  //
  //
  //            Buffer bufp;
  //            ecc.private_key().save(&bufp);
  //
  //            messages::KeyPriv keypriv;
  //            keypriv.set_type(messages::KeyType::ECP256K1);
  //            keypriv.set_data(bufp.data(),bufp.size());
  //
  //            t = "";
  //            messages::to_json(keypriv,&t);
  //            std::cout << i << " - " << t << std::endl;
  //
  //        }*
  //
  //  ledger->fork_test();  ///!< Delete Fork branche$
  //                        /** filter test
  //                          ledger::Ledger::Filter filter;
  //
  //                          messages::Hash id_transaction;
  //                          messages::from_json(
  //                              "{\"type\":\"SHA256\",\"data\":\"rYfXEdh9/"
  //                              "G7XmyYSPVKuVfcvuKFzHWFOZfpQ1dch71A=\"}",
  //                              &id_transaction);
  //                          std::string tt;
  //                          messages::to_json(id_transaction, &tt);
  //                          std::cout << "ID = " << tt << std::endl;
  //
  //                          filter.input_transaction_id(id_transaction);
  //                          filter.output_id(0);
  //
  //                          ledger->for_each(filter, [&](const
  //  messages::Transaction t) { std::string str; messages::to_json(t, &str);
  //                            std::cout << str << std::endl;
  //                            return true;
  //                          });*
  //  crypto::Ecc ecc({"../keys/key_0.priv"}, {"../keys/key_0.pub"});
  //  auto _wallet = std::vector{&ecc};
  //  auto io = std::shared_ptr<boost::asio::io_context>();
  //  consensus::PiiConsensus _PiiConsensus(io, ledger,
  //  std::make_shared<networking::Networking>(), 10);
  //  _PiiConsensus.show_results();
  //  //_PiiConsensus.show_owner(0, 10);
  //
  //  messages::Block block10;
  //  ledger->get_block(10, &block10);
  //
  //  std::cout << "Block 11 " << std::endl;
  //  messages::Block block11;
  //  auto author11 = load_key_pub(8);
  //  tooling::genblock::genblock_from_last_db_block(
  //      block11, ledger, 0, 11,
  //      std::make_optional<messages::KeyPub>(author11), 9);
  //  _PiiConsensus.add_block(block11);
  //
  //  std::cout << "Block 12 " << std::endl;
  //
  //  load_id_transaction(&block11);
  //  messages::Block block12;
  //  auto author12 = load_key_pub(4);
  //  tooling::genblock::genblock_from_block(block12, block11,
  //  std::time(nullptr),
  //                                         12, author12);
  //  _PiiConsensus.add_block(block12);
  //
  //  std::cout << "Block 13 " << std::endl;
  //  messages::Block block13;
  //  auto author13 = load_key_pub(1);
  //  tooling::genblock::genblock_from_block(block13, block10,
  //  std::time(nullptr),
  //                                         13, author13);
  //  _PiiConsensus.add_block(block13);
  //  std::cout << "Block 14 " << std::endl;
  //
  //  messages::Block block14;
  //  auto author14 = load_key_pub(3);
  //  tooling::genblock::genblock_from_last_db_block(
  //      block14, ledger, std::time(nullptr), 14,
  //      std::make_optional<messages::KeyPub>(author14), 13);
  //  _PiiConsensus.add_block(block14);
  //
  //  std::cout << "Block 15 " << std::endl;
  //  messages::Block block15;
  //  tooling::genblock::genblock_from_last_db_block(
  //      block15, ledger, std::time(nullptr), 15,
  //      std::make_optional<messages::KeyPub>(author13), 14);
  //  _PiiConsensus.add_block(block15);
  //
  //  load_id_transaction(&block15);
  //  std::cout << "Block 16 " << std::endl;
  //  messages::Block block16;
  //  auto author16 = load_key_pub(8);
  //  tooling::genblock::genblock_from_last_db_block(
  //      block16, ledger, std::time(nullptr), 16,
  //      std::make_optional<messages::KeyPub>(author16), 15);
  //  _PiiConsensus.add_block(block16);
  //  std::cout << "Block 17 " << std::endl;
  //  messages::Block block17;
  //  auto author17 = load_key_pub(6);
  //  tooling::genblock::genblock_from_last_db_block(
  //      block17, ledger, std::time(nullptr), 17,
  //      std::make_optional<messages::KeyPub>(author17), 15);
  //  _PiiConsensus.add_block(block17);
  //  std::cout << "Block 18 " << std::endl;
  //  messages::Block block18;
  //  auto author18 = load_key_pub(9);
  //  tooling::genblock::genblock_from_last_db_block(
  //      block18, ledger, std::time(nullptr), 18,
  //      std::make_optional<messages::KeyPub>(author18), 16);
  //  _PiiConsensus.add_block(block18);
  //
  //  std::cout << "Block 19 " << std::endl;
  //  messages::Block block19;
  //  auto author19 = load_key_pub(2);
  //  tooling::genblock::genblock_from_last_db_block(
  //      block19, ledger, std::time(nullptr), 19,
  //      std::make_optional<messages::KeyPub>(author19), 18);
  //  _PiiConsensus.add_block(block19);
  //
  //  _PiiConsensus.show_results();
  //  // _PiiConsensus.show_owner(0, 10);
  //*/
  return 0;
}
}  // namespace neuro
int main(int argc, char *argv[]) { return neuro::main(argc, argv); }
