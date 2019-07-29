#include "api/Rest.hpp"
#include <messages/Hasher.hpp>
#include "Bot.hpp"
#include "common/logger.hpp"
#include "ledger/Ledger.hpp"

namespace neuro::api {

Rest::Rest(const messages::config::Rest &config, Bot *bot)
    : Api::Api(bot),
      _httpEndpoint(std::make_shared<Http::Endpoint>(
          Address(Ipv4::any(), config.port()))) {
  init();
  start();
}

void Rest::init() {
  auto opts = Http::Endpoint::options()
                  .threads(1)
                  .maxRequestSize(1024 * 1024)  // 1Mio
                  .flags(Tcp::Options::ReuseAddr);
  _httpEndpoint->init(opts);
  setupRoutes();
}

void Rest::start() {
  _httpEndpoint->setHandler(_router.handler());
  _httpEndpoint->serveThreaded();
}

void Rest::shutdown() { _httpEndpoint->shutdown(); }

void Rest::send(Response &response, const messages::Packet &packet) {
  response.headers().add<Http::Header::AccessControlAllowOrigin>("*");
  response.send(Pistache::Http::Code::Ok, messages::to_json(packet));
}

void Rest::send(Response &response, const std::string &value) {
  response.headers().add<Http::Header::AccessControlAllowOrigin>("*");
  response.send(Pistache::Http::Code::Ok, value);
}

void Rest::bad_request(Response &response, const std::string &message) {
  response.headers().add<Http::Header::AccessControlAllowOrigin>("*");
  response.send(Pistache::Http::Code::Bad_Request, message);
}

void Rest::setupRoutes() {
  using namespace ::Pistache::Rest::Routes;
  Get(_router, "/balance/:key_pub", bind(&Rest::get_balance, this));
  Get(_router, "/ready", bind(&Rest::get_ready, this));
  Post(_router, "/create_transaction/:key_pub/:fees",
       bind(&Rest::get_create_transaction, this));
  Post(_router, "/publish", bind(&Rest::publish, this));*
  // Post(_router, "/list_transactions/:key_pub", bind(&Rest::get_unspent_transaction_list, this));
  Post(_router, "/transaction/", bind(&Rest::get_transaction, this));
  Post(_router, "/block/id", bind(&Rest::get_block_by_id, this));
  Get(_router, "/block/height/:height", bind(&Rest::get_block_by_height, this));
  Get(_router, "/last_blocks/:nb_blocks", bind(&Rest::get_last_blocks, this));
  Get(_router, "/total_nb_transactions",
      bind(&Rest::get_total_nb_transactions, this));
  Get(_router, "/total_nb_blocks", bind(&Rest::get_total_nb_blocks, this));
  Get(_router, "/peers", bind(&Rest::get_peers, this));
}

void Rest::get_balance(const Request &req, Response res) {
  messages::_KeyPub key_pub;
  key_pub.set_hex_data((req.param(":key_pub").as<std::string>()));
  const auto balance_amount = balance(key_pub);
  send(res, balance_amount);
}

void Rest::get_ready(const Request &req, Response res) { send(res, "{ok: 1}"); }

void Rest::get_transaction(const Request &req, Response res) {
  messages::Hash transaction_id;
  if (!messages::from_json(req.body(), &transaction_id)) {
    bad_request(res, "could not parse body");
  }
  messages::Transaction transaction = Api::transaction(transaction_id);
  send(res, transaction);
}

void Rest::get_create_transaction(const Request &req, Response res) {
  messages::_KeyPub key_pub;
  key_pub.set_hex_data(req.param(":key_pub").as<std::string>());

  messages::Transaction transaction;
  messages::from_json(req.body(), &transaction);
  transaction.add_inputs()->mutable_key_pub()->CopyFrom(key_pub);

  transaction.mutable_id()->set_type(messages::Hash::SHA256);
  transaction.mutable_id()->set_data("");

  const auto transaction_opt = messages::to_buffer(transaction);
  if (!transaction_opt) {
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
      Buffer(publish_message.transaction(), Buffer::InputType::HEX);
  const auto signature =
      Buffer(publish_message.signature(), Buffer::InputType::HEX);

  messages::Transaction transaction;
  if (!messages::from_buffer(transaction_bin, &transaction)) {
    bad_request(res, "Could not parse transaction");
    return;
  }

  crypto::KeyPub key_pub(publish_message.key_pub());
  auto input = transaction.mutable_inputs(0);
  auto input_signature = input->mutable_signature();
  input_signature->set_type(messages::Hash::SHA256);
  key_pub.save(input->mutable_key_pub());
  input_signature->set_data(signature.str());

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

void Rest::get_block_by_id(const Rest::Request &req, Rest::Response res) {
  messages::BlockID block_id;
  if (!messages::from_json(req.body(), &block_id)) {
    bad_request(res, "could not parse body");
  }
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

void Rest::get_total_nb_transactions(const Rest::Request &req,
                                     Rest::Response res) {
  send(res, std::to_string(Api::total_nb_transactions()));
}

void Rest::get_total_nb_blocks(const Rest::Request &req, Rest::Response res) {
  send(res, std::to_string(Api::total_nb_blocks()));
}

void Rest::get_peers(const Rest::Request &request, Rest::Response res) {
  send(res, messages::to_json(Api::peers()));
}

void Rest::get_status(const Rest::Request &req, Rest::Response res) {
  auto status = _monitor.complete_status();
  send(res, messages::to_json(status));
}

Rest::~Rest() { shutdown(); }

}  // namespace neuro::api
