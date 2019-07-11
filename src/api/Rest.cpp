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
//  _root->add("generate_keys", generate_keys_route);
//
//  if (_config.has_faucet_amount()) {
//    _root->add("faucet_send", faucet_send_route);
//  }

Rest::Rest(const messages::config::Rest &config, Bot *bot)
    : Api::Api(bot), _httpEndpoint(std::make_shared<Http::Endpoint>(
                         Address(Ipv4::any(), config.port()))) {
  init();
  start();
}

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

void Rest::send(Response &response, const messages::Packet &packet) {
  response.headers().add<Http::Header::AccessControlAllowOrigin>("*");
  response.send(Pistache::Http::Code::Ok, messages::to_json(packet));
}

void Rest::send(Response &response, const std::string &value) {
  response.headers().add<Http::Header::AccessControlAllowOrigin>("*");
  response.send(Pistache::Http::Code::Ok, value);
}

void Rest::bad_request(Response &response, const std::string message) {
  response.headers().add<Http::Header::AccessControlAllowOrigin>("*");
  response.send(Pistache::Http::Code::Bad_Request, message);
}

void Rest::setupRoutes() {
  using namespace ::Pistache::Rest::Routes;
  // Routes::Post(router, "/record/:name/:value?",
  // Routes::bind(&StatsEndpoint::doRecordMetric, this));
  Get(_router, "/balance/:address", bind(&Rest::get_balance, this));
  Get(_router, "/ready", bind(&Rest::get_ready, this));
  Post(_router, "/create_transaction/:address/:fees",
       bind(&Rest::get_create_transaction, this));
  Post(_router, "/publish", bind(&Rest::publish, this));
  Get(_router, "/list_transactions/:address", bind(&Rest::get_unspent_transaction_list, this));
  Get(_router, "/transaction/:id", bind(&Rest::get_transaction, this));
  Get(_router, "/block/id/:id", bind(&Rest::get_block_by_id, this));
  Get(_router, "/block/height/:height", bind(&Rest::get_block_by_height, this));
  Get(_router, "/last_blocks/:nb_blocks", bind(&Rest::get_last_blocks, this));
  Get(_router, "/total_nb_transactions", bind(&Rest::get_total_nb_transactions, this));
  Get(_router, "/total_nb_blocks", bind(&Rest::get_total_nb_blocks, this));
  Get(_router, "/peers", bind(&Rest::get_peers, this));
}

void Rest::get_balance(const Request& req, Response res) {
  const messages::Address address(req.param(":address").as<std::string>());
  const auto balance_amount = balance(address);
  send(res, balance_amount);
}

void  Rest::get_ready(const Request& req, Response res) {
  send(res, "{ok: 1}");
}

void Rest::get_transaction(const Request& req, Response res) {
  messages::Hasher transaction_id(req.param(":id").as<std::string>());
  messages::Transaction transaction = Api::transaction(transaction_id);
  send(res, transaction);
}

void Rest::get_create_transaction(const Request& req, Response res) {
  const messages::Address address(req.param(":address").as<std::string>());
  const auto fees = req.param(":fees").as<uint64_t>();

  messages::Transaction transaction;
  messages::from_json(req.body(), &transaction);
  if(!set_inputs(&transaction, address, messages::NCCAmount{fees})) {
    bad_request(res, "Could not set inputs, insuffisant funds?");
    return;
  }

  transaction.mutable_id()->set_type(messages::Hash::SHA256);
  transaction.mutable_id()->set_data("");

  const auto transaction_opt = messages::to_buffer(transaction);
  if(!transaction_opt) {
    bad_request(res, "Could not serialize transaction");
    return;
  }
  send(res, transaction_opt->to_hex());
}

void Rest::publish(const Request &req, Response res) {
  messages::Publish publish_message;
  if (!messages::from_json(req.body(), &publish_message)) {
    bad_request(res, "Could not parse body");
  }

  const auto transaction_bin =
      Buffer(publish_message.transaction(),
             Buffer::InputType::HEX);
  const auto signature = Buffer(publish_message.signature(),
                                Buffer::InputType::HEX);

  messages::Transaction transaction;
  if (!messages::from_buffer(transaction_bin, &transaction)) {
    bad_request(res, "Could not parse transaction");
    return;
  }

  crypto::KeyPub key_pub(publish_message.key_pub());
  auto input_signature = transaction.add_signatures();
  input_signature->mutable_signature()->set_type(messages::Hash::SHA256);
  key_pub.save(input_signature->mutable_key_pub());
  input_signature->mutable_signature()->set_data(signature.data(),
                                                 signature.size());

  if (!crypto::verify(transaction)) {
    bad_request(res, "Bad signature");
    return;
  }

  if (!transaction_publish(transaction)) {
    bad_request(res, "Could not publish transaction");
    return;
  }

  res.headers().add<Http::Header::AccessControlAllowOrigin>("*");
  res.send(Pistache::Http::Code::Ok);
}

void Rest::get_unspent_transaction_list(const Request& req, Response res) {
  const messages::Address address(req.param(":address").as<std::string>());
  auto unspent_transaction_list = list_unspent_transaction(address);
  send(res, unspent_transaction_list);
}

void Rest::get_block_by_id(const Rest::Request &req, Rest::Response res) {
  messages::Hasher block_id(req.param(":id").as<std::string>());
  auto block = Api::block(block_id);
  send(res, block);
}

void Rest::get_block_by_height(const Rest::Request &req, Rest::Response res) {
  const auto block_height(req.param(":height").as<messages::BlockHeight>());
  auto block = Api::block(block_height);
  send(res, block);
}

void Rest::get_last_blocks(const Rest::Request &req, Rest::Response res) {
  auto nb_blocks = req.param(":nb_blocks").as<std::size_t>();
  auto blocks = Api::last_blocks(nb_blocks);
  send(res, blocks);
}

void Rest::get_total_nb_transactions(const Rest::Request &req, Rest::Response res) {
  send(res, std::to_string(Api::total_nb_transactions()));
}

void Rest::get_total_nb_blocks(const Rest::Request &req, Rest::Response res) {
  send(res, std::to_string(Api::total_nb_blocks()));
}

void Rest::get_peers(const Rest::Request& request, Rest::Response res) {
  send(res, to_json(Api::peers()));
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
