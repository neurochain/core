#include "api/Rest.hpp"
#include "Bot.hpp"
#include "common/logger.hpp"
#include "ledger/Ledger.hpp"
#include "messages/Address.hpp"
#include <messages/Hasher.hpp>

namespace neuro::api {

//  const auto generate_keys_route = [this](Onion::Request &req,
//                                          Onion::Response &res) {
//    messages::GeneratedKeys generated_keys = generate_keys();
//    res << generated_keys;
//    return OCS_PROCESSED;
//  };
//
//  const auto faucet_send_route = [this](Onion::Request &req,
//                                        Onion::Response &res) {
//    const auto faucet_amount = _config.faucet_amount();
//    const auto address_str = req.query("address", "");
//    LOG_INFO << "ADDRESS " << address_str;
//    const messages::Address address(address_str);
//    if (_ledger->has_received_transaction(address)) {
//      res << "{\"error\": \"The given address has already received some "
//             "coins.\"}";
//      return OCS_PROCESSED;
//    } else {
//      messages::NCCAmount amount;
//      amount.set_value(faucet_amount);
//      messages::Transaction transaction =
//          _ledger->build_transaction(address, amount, _keys->key_priv());
//      _bot->publish_transaction(transaction);
//      res << transaction;
//      return OCS_PROCESSED;
//    }
//  };
//
//  const auto get_block_route = [this](Onion::Request &req,
//                                      Onion::Response &res) {
//    const auto block_id_str = req.query("block_id", "");
//    const messages::Hasher block_id = load_hash(block_id_str);
//    messages::Block block;
//    if (block_id_str != "") {
//      _ledger->get_block(block_id, &block);
//    } else {
//      const auto block_height_str = req.query("height", "");
//      if (block_height_str != "") {
//        int block_height = std::stoi(block_height_str);
//        _ledger->get_block(block_height, &block);
//      }
//    }
//    res << block;
//    return OCS_PROCESSED;
//  };
//
//  const auto get_last_blocks_route = [this](Onion::Request &req,
//                                            Onion::Response &res) {
//    const auto nb_blocks_str = req.query("nb_blocks", "10");
//    int nb_blocks = std::stoi(nb_blocks_str);
//    std::vector<messages::Block> blocks_vector =
//        _ledger->get_last_blocks(nb_blocks);
//    messages::Blocks blocks;
//    for (auto block : blocks_vector) {
//      blocks.add_blocks()->CopyFrom(block);
//    }
//    res << blocks;
//    return OCS_PROCESSED;
//  };
//
//  const auto total_nb_transactions_route = [this](Onion::Request &req,
//                                                  Onion::Response &res) {
//    res << "{\"totalNbTransactions\":" << _ledger->total_nb_transactions()
//        << "}\n";
//    return OCS_PROCESSED;
//  };
//
//  const auto total_nb_blocks_route = [this](Onion::Request &req,
//                                            Onion::Response &res) {
//    res << "{\"totalNbBlocks\":" << _ledger->total_nb_blocks() << "}\n";
//    return OCS_PROCESSED;
//  };
//
//  _root->add("generate_keys", generate_keys_route);
//  if (_config.has_faucet_amount()) {
//    _root->add("faucet_send", faucet_send_route);
//  }
//  _root->add("get_block", get_block_route);
//  _root->add("get_last_blocks", get_last_blocks_route);
//  _root->add("total_nb_transactions", total_nb_transactions_route);
//  _root->add("total_nb_blocks", total_nb_blocks_route);

void Rest::init() {
  auto opts =
      Http::Endpoint::options().threads(1).maxRequestSize(1024 * 1024); // 1Mio
  _httpEndpoint->init(opts);
  setupRoutes();
}

void Rest::start() {
  _httpEndpoint->setHandler(_router.handler());
  _httpEndpoint->serveThreaded();
}

void Rest::shutdown() {
  _httpEndpoint->shutdown();
}

void Rest::setupRoutes() {
  using namespace Rest::Routes;
  // Routes::Post(router, "/record/:name/:value?",
  // Routes::bind(&StatsEndpoint::doRecordMetric, this));
  Get(_router, "/balance/:address", bind(&Rest::get_balance, this));
  Get(_router, "/ready", bind(&Rest::get_ready, this));
  Post(_router, "/create_transaction/:address/:fees",
       bind(&Rest::get_create_transaction, this));
  Post(_router, "/publish", bind(&Rest::publish, this));
  Get(_router, "/list_transactions/:address", bind(&Rest::get_unspent_transaction_list, this));
  Get(_router, "/get_transaction/:id", bind(&Rest::get_transaction, this));
}

void Rest::get_balance(const Request& req, Response res) {
  const messages::Address address(req.param(":address").as<std::string>());
  const auto balance_amount = balance(address);
  res.send(Pistache::Http::Code::Ok, to_json(balance_amount));
}

void  Rest::get_ready(const Request& req, Response res) {
  res.send(Pistache::Http::Code::Ok, "{ok: 1}");
}

void Rest::get_transaction(const Request& req, Response res) {
  messages::Hasher transaction_id(req:param(":id").as<std::string>());
  messages::Transaction transaction = Api::transaction(transaction_id);
  res.send(Pistache::Http::Code::Ok, to_json(transaction));
}

void Rest::get_create_transaction(const Request& req, Response res) {
  const messages::Address address(req.param(":address").as<std::string>());
  const auto fees = req.param(":fees").as<uint64_t>();

  messages::Transaction transaction;
  messages::from_json(req.body(), &transaction);
  if(!set_inputs(&transaction, address, messages::NCCAmount{fees})) {
    res.send(Pistache::Http::Code::Bad_Request, "Could not set inputs, insuffisant funds?");
    return;
  }

  transaction.mutable_id()->set_type(messages::Hash::SHA256);
  transaction.mutable_id()->set_data("");

  const auto transaction_opt = messages::to_buffer(transaction);
  if(!transaction_opt) {
    res.send(Pistache::Http::Code::Bad_Request, "Could not serialize transaction");
    return;
  }
  res.send(Pistache::Http::Code::Ok, transaction_opt->to_hex());
}

void Rest::publish(const Request &req, Response res) {
  messages::Publish publish_message;
  if (!messages::from_json(req.body(), &publish_message)) {
    res.send(Pistache::Http::Code::Bad_Request,
                  "Could not parse body");
  }

  const auto transaction_bin =
      Buffer(publish_message.transaction(),
             Buffer::InputType::HEX);
  const auto signature = Buffer(publish_message.signature(),
                                Buffer::InputType::HEX);

  messages::Transaction transaction;
  if (!messages::from_buffer(transaction_bin, &transaction)) {
    res.send(Pistache::Http::Code::Bad_Request,
                  "Could not parse transaction");
    return;
  }

  crypto::KeyPub key_pub(publish_message.key_pub());
  auto input_signature = transaction.add_signatures();
  input_signature->mutable_signature()->set_type(messages::Hash::SHA256);
  key_pub.save(input_signature->mutable_key_pub());
  input_signature->mutable_signature()->set_data(signature.data(),
                                                 signature.size());

  if (!crypto::verify(transaction)) {
    res.send(Pistache::Http::Code::Bad_Request, "Bad signature");
    return;
  }

  if (!transaction_publish(transaction)) {
    res.send(Pistache::Http::Code::Bad_Request,
         "Could not publish transaction");
    return;
  }

  res.send(Pistache::Http::Code::Ok);
}

void Rest::get_unspent_transaction_list(const Request& req, Response res) {
  const messages::Address address(req.param(":address").as<std::string>());
  auto unspent_transaction_list = list_unspent_transaction(address);
  res.send(Pistache::Http::Code::Ok, to_json(unspent_transaction_list));
}

Rest::Rest(const messages::config::Rest &config, Bot *bot)
    : Api::Api(bot), _httpEndpoint(std::make_shared<Http::Endpoint>(
                         Address(Ipv4::any(), config.port()))) {
  init();
  start();
}

Rest::~Rest() {
  shutdown();
}

//
//messages::Transaction Rest::build_transaction(
//    const messages::TransactionToPublish &transaction_to_publish) const {
//  messages::Transaction transaction;
//
//  // Load keys
//  auto buffer = Buffer(transaction_to_publish.key_priv());
//  const auto random_pool = std::make_shared<CryptoPP::AutoSeededRandomPool>();
//  auto key_priv = crypto::KeyPriv(random_pool);
//  key_priv.load(buffer);
//
//  std::vector<messages::Output> outputs;
//  auto outputs_to_publish = transaction_to_publish.outputs();
//  for (auto output : outputs_to_publish) {
//    outputs.push_back(output);
//  }
//
//  const crypto::KeyPub key_pub = key_priv.make_key_pub();
//  const auto address = messages::Address(key_pub);
//  const auto ecc = crypto::Ecc(key_priv, key_pub);
//  std::vector<const crypto::Ecc *> keys = {&ecc};
//
//  // Process the outputs and lookup their output_id to build the inputs
//  std::vector<messages::Address> transaction_ids;
//  auto transaction_ids_str = transaction_to_publish.transactions_ids();
//  for (auto transaction_id_str : transaction_ids_str) {
//    messages::Address transaction_id;
//    transaction_id.set_type(messages::Hash_Type_SHA256);
//    transaction_id.set_data(transaction_id_str);
//    transaction_ids.push_back(transaction_id);
//  }
//
//  return _ledger->build_transaction(transaction_ids, outputs, key_priv,
//                                    transaction_to_publish.fees());
//}
//
//messages::GeneratedKeys Rest::generate_keys() const {
//  messages::GeneratedKeys generated_keys;
//  crypto::Ecc ecc;
//  messages::_KeyPub key_pub;
//  ecc.key_pub().save(&key_pub);
//  messages::_KeyPriv key_priv;
//  ecc.key_priv().save(&key_priv);
//  generated_keys.mutable_key_priv()->CopyFrom(key_priv);
//  generated_keys.mutable_key_pub()->CopyFrom(key_pub);
//  generated_keys.mutable_address()->CopyFrom(
//      messages::Hasher(ecc.key_pub()));
//  return generated_keys;
//}


}  // namespace neuro::api
