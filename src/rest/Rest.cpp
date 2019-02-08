#include <dsa.h>
#include <onion/exportlocal.h>
#include <onion/extrahandlers.hpp>
#include <onion/request.hpp>
#include <onion/response.hpp>

#include "Bot.hpp"
#include "common/logger.hpp"
#include "crypto/Sign.hpp"
#include "ledger/Ledger.hpp"
#include "rest/Rest.hpp"

namespace neuro {
namespace rest {

Rest::Rest(Bot *bot, std::shared_ptr<ledger::Ledger> ledger,
           std::shared_ptr<crypto::Ecc> keys,
           std::shared_ptr<consensus::Consensus> consensus,
           const messages::config::Rest &config)
    : _bot(bot),
      _ledger(ledger),
      _keys(keys),
      _consensus(consensus),
      _config(config),
      _port(_config.port()),
      _static_path(_config.static_path()),
      _server(O_POOL) {
  _server.setPort(_port);
  _server.setHostname("0.0.0.0");
  _root = std::make_unique<Onion::Url>(&_server);

  const auto list_transactions_route = [this](Onion::Request &req,
                                              Onion::Response &res) {
    const auto address = req.query("address", "");
    LOG_INFO << "ADDRESS " << address;
    res << list_transactions(address);
    return OCS_PROCESSED;
  };

  const auto publish_transaction_route = [this](Onion::Request &req,
                                                Onion::Response &res) {
    onion_request *c_req = req.c_handler();
    const onion_block *dreq = onion_request_get_data(c_req);
    std::string post_data = onion_block_data(dreq);
    messages::TransactionToPublish transaction_to_publish;
    messages::from_json(post_data, &transaction_to_publish);
    auto transaction = build_transaction(transaction_to_publish);
    _bot->publish_transaction(transaction);
    res << transaction;
    return OCS_PROCESSED;
  };

  const auto generate_keys_route = [this](Onion::Request &req,
                                          Onion::Response &res) {
    messages::GeneratedKeys generated_keys = generate_keys();
    res << generated_keys;
    return OCS_PROCESSED;
  };

  const auto faucet_send_route = [this](Onion::Request &req,
                                        Onion::Response &res) {
    const auto faucet_amount = _config.faucet_amount();
    const auto address_str = req.query("address", "");
    LOG_INFO << "ADDRESS " << address_str;
    const messages::Address address = load_hash(address_str);
    if (_ledger->has_received_transaction(address)) {
      res << "{\"error\": \"The given address has already received some "
             "coins.\"}";
      return OCS_PROCESSED;
    } else {
      messages::NCCAmount amount;
      amount.set_value(faucet_amount);
      messages::Transaction transaction =
          _ledger->build_transaction(address, amount, _keys->private_key());
      _bot->publish_transaction(transaction);
      res << transaction;
      return OCS_PROCESSED;
    }
  };

  const auto get_transaction_route = [this](Onion::Request &req,
                                            Onion::Response &res) {
    const auto transaction_id_str = req.query("transaction_id", "");
    const messages::Hasher transaction_id = load_hash(transaction_id_str);
    messages::Transaction transaction;
    _ledger->get_transaction(transaction_id, &transaction);
    res << transaction;
    return OCS_PROCESSED;
  };

  const auto get_block_route = [this](Onion::Request &req,
                                      Onion::Response &res) {
    const auto block_id_str = req.query("block_id", "");
    const messages::Hasher block_id = load_hash(block_id_str);
    messages::Block block;
    if (block_id_str != "") {
      _ledger->get_block(block_id, &block);
    } else {
      const auto block_height_str = req.query("height", "");
      if (block_height_str != "") {
        int block_height = std::stoi(block_height_str);
        _ledger->get_block(block_height, &block);
      }
    }
    res << block;
    return OCS_PROCESSED;
  };

  const auto get_last_blocks_route = [this](Onion::Request &req,
                                            Onion::Response &res) {
    const auto nb_blocks_str = req.query("nb_blocks", "10");
    int nb_blocks = std::stoi(nb_blocks_str);
    std::vector<messages::Block> blocks_vector =
        _ledger->get_last_blocks(nb_blocks);
    messages::Blocks blocks;
    for (auto block : blocks_vector) {
      blocks.add_blocks()->CopyFrom(block);
    }
    res << blocks;
    return OCS_PROCESSED;
  };

  const auto total_nb_transactions_route = [this](Onion::Request &req,
                                                  Onion::Response &res) {
    res << "{\"totalNbTransactions\":" << _ledger->total_nb_transactions()
        << "}\n";
    return OCS_PROCESSED;
  };

  const auto total_nb_blocks_route = [this](Onion::Request &req,
                                            Onion::Response &res) {
    res << "{\"totalNbBlocks\":" << _ledger->total_nb_blocks() << "}\n";
    return OCS_PROCESSED;
  };

  _root->add("list_transactions", list_transactions_route);
  _root->add("publish_transaction", publish_transaction_route);
  _root->add("generate_keys", generate_keys_route);
  if (_config.has_faucet_amount()) {
    _root->add("faucet_send", faucet_send_route);
  }
  _root->add("get_transaction", get_transaction_route);
  _root->add("get_block", get_block_route);
  _root->add("get_last_blocks", get_last_blocks_route);
  _root->add("total_nb_transactions", total_nb_transactions_route);
  _root->add("total_nb_blocks", total_nb_blocks_route);

  /* NEW REST API */

  /**
   * Get Unspend Transaction
   * For Pub Key
   */
  const auto transactions_route = [&](Onion::Request &request,
                                      Onion::Response &response) {
    this->add_cors(response);
    if ((onion_request_get_flags(request.c_handler()) & OR_OPTIONS) ==
        OR_OPTIONS) {
      response << "done";  // TO DO #1 Static
      return OCS_PROCESSED;
    }
    if ((onion_request_get_flags(request.c_handler()) & OR_POST) == OR_POST) {
      messages::PublicKey publickey;
      this->get_post(request, &publickey);  // TO DO #2 check input
      response << this->pubkey_transactions(publickey);
      return OCS_PROCESSED;
    }

    response << "No Allow";
    return OCS_PROCESSED;
  };

  /**
   *  Build Raw Transaction for signature
   *  For Pubkey and Transactions Unspend ID
   * */
  const auto build_raw_transactions = [&](Onion::Request &request,
                                          Onion::Response &response) {
    this->add_cors(response);

    if ((onion_request_get_flags(request.c_handler()) & OR_OPTIONS) ==
        OR_OPTIONS) {
      response << "done";
      return OCS_PROCESSED;
    }

    if ((onion_request_get_flags(request.c_handler()) & OR_POST) == OR_POST) {
      messages::BuildTransaction buildtransaction;
      this->get_post(request, &buildtransaction);
      response << this->populate_transaction(buildtransaction);
      return OCS_PROCESSED;
    }
    response << "No Allow";
    return OCS_PROCESSED;
  };

  const auto send_transactions = [&](Onion::Request &request,
                                     Onion::Response &response) {
    this->add_cors(response);
    if ((onion_request_get_flags(request.c_handler()) & OR_OPTIONS) ==
        OR_OPTIONS) {
      response << "done";
      return OCS_PROCESSED;
    }

    if ((onion_request_get_flags(request.c_handler()) & OR_POST) == OR_POST) {
      messages::BuildTransaction buildtransaction;
      this->get_post(request, &buildtransaction);
      if (crypto::verify(buildtransaction.transaction())) {
        messages::set_transaction_hash(buildtransaction.mutable_transaction());
        // TO DO #4 add transaction in pool of bot
        _bot->publish_transaction(buildtransaction.transaction());
        response << buildtransaction.transaction().id();
      } else {
        response << "{\"status\":false}";  // TO DO #3 messages protbuf ??
      }
      return OCS_PROCESSED;
    }

    response << "false";
    return OCS_PROCESSED;
  };

  const auto get_addr_from_x_y = [&](Onion::Request &request,
                                     Onion::Response &response) {
    this->add_cors(response);

    if ((onion_request_get_flags(request.c_handler()) & OR_OPTIONS) ==
        OR_OPTIONS) {
      response << "done";
      return OCS_PROCESSED;
    }

    if ((onion_request_get_flags(request.c_handler()) & OR_POST) == OR_POST) {
      messages::PublicKey publickey;
      this->get_post(request, &publickey);
      response << this->pubkey_addr(publickey);
      return OCS_PROCESSED;
    }

    response << "No Allow";
    return OCS_PROCESSED;
  };
  /**
   * For tests
   */
  const auto build_coinbase = [&](Onion::Request &request,
                                  Onion::Response &response) {
    // TO DO #5 Remove after testing
    std::string post_data =
        onion_block_data(onion_request_get_data(request.c_handler()));
    messages::PublicKey publickey;
    messages::from_json(post_data, &publickey);
    crypto::EccPub eccpubkey;
    eccpubkey.load(publickey);
    messages::Hasher address(eccpubkey);

    messages::TaggedTransaction taggedtransaction;
    messages::NCCAmount ncc;
    ncc.set_value(100000);

    consensus::TransactionPool tp(_ledger);
    tp.coinbase(taggedtransaction.mutable_transaction(), address, ncc);
    LOG_DEBUG << taggedtransaction.transaction().id();

    messages::BlockHeader lastblock;
    _ledger->get_last_block_header(&lastblock);
    taggedtransaction.mutable_block_id()->CopyFrom(lastblock.id());

    _ledger->add_transaction(taggedtransaction);

    response << ncc;
    return OCS_PROCESSED;
  };
  const auto open_wallet = [&](Onion::Request &request,
                               Onion::Response &response) {
    this->add_cors(response);
    if ((onion_request_get_flags(request.c_handler()) & OR_OPTIONS) ==
        OR_OPTIONS) {
      response << "done";
      return OCS_PROCESSED;
    }

    response << "{\"status\": true }";
    return OCS_PROCESSED;
  };

  _root->add("open_wallet", open_wallet);
  _root->add("transactions", transactions_route);
  _root->add("build_transactions", build_raw_transactions);
  _root->add("get_addr_x_y", get_addr_from_x_y);
  _root->add("send_transactions", send_transactions);
  _root->add("build_coinbase", build_coinbase);

  serve_folder("^static/", "static");
  serve_file("", "index.html");
  serve_file("index.html");
  serve_file("asset-manifest.json");
  serve_file("favicon.ico");
  serve_file("manifest.json");
  serve_file("service-worker.js");
  _thread = std::thread([this]() { _server.listen(); });
}

void Rest::add_cors(Onion::Response &response) {
  auto res_c = response.c_handler();
  onion_response_set_header(res_c, "Access-Control-Allow-Origin", "*");
  onion_response_set_header(res_c, "Access-Control-Allow-Credentials", "true");
  onion_response_set_header(
      res_c, "Access-Control-Allow-Headers",
      "Access-Control-Allow-Headers, Origin,Accept, X-Requested-With, "
      "Content-Type, Access-Control-Request-Method, "
      "Access-Control-Request-Headers");
  onion_response_set_header(res_c, "Access-Control-Allow-Method",
                            "OPTIONS,POST");
}

bool Rest::get_post(Onion::Request &request, messages::Packet *packet) {
  std::string post_data =
      onion_block_data(onion_request_get_data(request.c_handler()));
  messages::from_json(post_data, packet);
  return true;
}

messages::Hasher Rest::pubkey_addr(messages::PublicKey &pubkey) {
  crypto::EccPub eccpubkey;
  eccpubkey.load(pubkey);
  messages::Hasher address(eccpubkey);
  return address;
}

std::string Rest::pubkey_transactions(messages::PublicKey &pubkey) {
  messages::Hasher address = pubkey_addr(pubkey);
  const auto transactions = _ledger->list_transactions(address);
  return messages::to_json(transactions);
}

std::string Rest::populate_transaction(
    messages::BuildTransaction &buildtransaction) {
  crypto::EccPub eccpubkey;
  eccpubkey.load(buildtransaction.publickey());
  messages::Hasher address(eccpubkey);

  _ledger->add_change(buildtransaction.mutable_transaction(), address);
  Buffer transaction_serialized;
  messages::to_buffer(buildtransaction.transaction(), &transaction_serialized);
  std::stringstream ss;
  ss << transaction_serialized;
  buildtransaction.set_raw_transaction(ss.str());
  auto *signature = buildtransaction.mutable_transaction()->add_signatures();
  signature->mutable_signature()->set_type(messages::Hash::SHA256);
  signature->mutable_signature()->set_data("");
  messages::KeyPub keypub;
  eccpubkey.save(&keypub);
  signature->mutable_key_pub()->CopyFrom(keypub);
  return messages::to_json(buildtransaction);
}
void Rest::serve_file(const std::string &filename) {
  _root->add(filename,
             Onion::Shortcuts::static_file((_static_path + filename).c_str()));
}

void Rest::serve_file(const std::string &route, const std::string &filename) {
  _root->add(route,
             Onion::Shortcuts::static_file((_static_path + filename).c_str()));
}

void Rest::serve_folder(const std::string &route,
                        const std::string &foldername) {
  _root->add(route, onion_handler_export_local_new(
                        (_static_path + foldername).c_str()));
}

messages::Hasher Rest::load_hash(const std::string &hash_str) const {
  messages::Hasher result;
  std::stringstream hash_json;
  hash_json << "{\"type\": \"SHA256\", \"data\": \"" << hash_str << "\"}";
  messages::from_json(hash_json.str(), &result);
  return result;
}

std::string Rest::list_transactions(const std::string &address_str) const {
  const messages::Hasher address = load_hash(address_str);
  std::vector<messages::UnspentTransaction> unspent_transactions =
      _ledger->list_unspent_transactions(address);
  messages::RestUnspentTransactions rest_unspent_transactions;
  for (auto unspent_transaction : unspent_transactions) {
    auto rest_unspent_transaction =
        rest_unspent_transactions.add_unspent_transactions();
    rest_unspent_transaction->set_transaction_id(
        unspent_transaction.transaction_id().data());
    rest_unspent_transaction->set_value(
        std::to_string(unspent_transaction.value().value()));
  }
  std::string result;
  messages::to_json(rest_unspent_transactions, &result);
  return result;
}

messages::Transaction Rest::build_transaction(
    const messages::TransactionToPublish &transaction_to_publish) const {
  messages::Transaction transaction;

  // Load keys
  auto buffer = Buffer(transaction_to_publish.key_priv());
  const auto random_pool = std::make_shared<CryptoPP::AutoSeededRandomPool>();
  auto key_priv = crypto::EccPriv(random_pool);
  key_priv.load(buffer);

  std::vector<messages::Output> outputs;
  auto outputs_to_publish = transaction_to_publish.outputs();
  for (auto output : outputs_to_publish) {
    outputs.push_back(output);
  }

  const crypto::EccPub key_pub = key_priv.make_public_key();
  const auto address = messages::Address(key_pub);
  const auto ecc = crypto::Ecc(key_priv, key_pub);
  std::vector<const crypto::Ecc *> keys = {&ecc};

  // Process the outputs and lookup their output_id to build the inputs
  std::vector<messages::Address> transaction_ids;
  auto transaction_ids_str = transaction_to_publish.transactions_ids();
  for (auto transaction_id_str : transaction_ids_str) {
    messages::Address transaction_id;
    transaction_id.set_type(messages::Hash_Type_SHA256);
    transaction_id.set_data(transaction_id_str);
    transaction_ids.push_back(transaction_id);
  }

  return _ledger->build_transaction(transaction_ids, outputs, key_priv,
                                    transaction_to_publish.fees());
}

messages::GeneratedKeys Rest::generate_keys() const {
  messages::GeneratedKeys generated_keys;
  crypto::Ecc ecc;
  messages::KeyPub key_pub;
  ecc.public_key().save(&key_pub);
  messages::KeyPriv key_priv;
  ecc.private_key().save(&key_priv);
  generated_keys.mutable_key_priv()->CopyFrom(key_priv);
  generated_keys.mutable_key_pub()->CopyFrom(key_pub);
  generated_keys.mutable_address()->CopyFrom(
      messages::Hasher(ecc.public_key()));
  return generated_keys;
}

void Rest::join() { _thread.join(); }

void Rest::stop() {
  _server.listenStop();
  _thread.join();
}
}  // namespace rest
}  // namespace neuro
