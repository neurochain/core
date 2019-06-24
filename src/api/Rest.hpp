#ifndef NEURO_SRC_API_REST_HPP
#define NEURO_SRC_API_REST_HPP

#include "common/types.hpp"
#include "api/Api.hpp"

#include <memory>
#include <pistache/endpoint.h>
#include <pistache/http.h>
#include <pistache/router.h>
#include <thread>

using namespace Pistache;

namespace neuro {
class Bot;
namespace api {

class Rest : public Api {
 private:
   //  Api *_api;
   std::shared_ptr<Http::Endpoint> httpEndpoint;
  Pistache::Rest::Router router;

  void init() {
    auto opts = Http::Endpoint::options()
                .threads(1)
                .maxRequestSize(1048576)  // 1Mio
                .flags(Tcp::Options::InstallSignalHandler);
    httpEndpoint->init(opts);
    setupRoutes();
    start();
  }

  void start() {
    httpEndpoint->setHandler(router.handler());
    httpEndpoint->serveThreaded();
  }

  void shutdown() {
    httpEndpoint->shutdown();
  }

  void setupRoutes() {
    using namespace Rest;

    // Routes::Post(router, "/record/:name/:value?", Routes::bind(&StatsEndpoint::doRecordMetric, this));
    Routes::Get(router, "/balance/:address", Routes::bind(&Rest::get_balance, this));
    Routes::Get(router, "/ready", Routes::bind(&Rest::get_ready, this));
    Routes::Post(router, "/create_transaction/:address/:fees",
                 Routes::bind(&Rest::get_create_transaction, this));
    Routes::Post(router, "/publish", Routes::bind(&Rest::publish, this));
  }

  void get_balance(const Pistache::Rest::Request& request, Pistache::Http::ResponseWriter response) {
    const messages::Address address (request.param(":address").as<std::string>());
    const auto balance_amount = balance(address);
    response.send(Pistache::Http::Code::Ok, to_json(balance_amount));
  }

  void get_ready(const Pistache::Rest::Request& request, Pistache::Http::ResponseWriter response) {
    response.send(Pistache::Http::Code::Ok, "{ok: 1}");
  }

  void get_create_transaction (const Pistache::Rest::Request& request, Pistache::Http::ResponseWriter response) {
    const messages::Address address (request.param(":address").as<std::string>());
    const auto fees = request.param(":fees").as<uint64_t>();
    
    messages::Transaction transaction;
    messages::from_json(request.body(), &transaction);
    if(!set_inputs(&transaction, address, messages::NCCAmount{fees})) {
      response.send(Pistache::Http::Code::Bad_Request, "Could not set inputs, insuffisant funds?");
      return;
    }

    transaction.mutable_id()->set_type(messages::Hash::SHA256);
    transaction.mutable_id()->set_data("");

    const auto transaction_opt = messages::to_buffer(transaction);
    if(!transaction_opt) {
      response.send(Pistache::Http::Code::Bad_Request, "Could not serialize transaction");
      return;
    }
    response.send(Pistache::Http::Code::Ok, transaction_opt->to_hex());
  }

  void publish(const Pistache::Rest::Request &request,
               Pistache::Http::ResponseWriter response) {
    messages::Publish publish_message;
    if (!messages::from_json(request.body(), &publish_message)) {
      response.send(Pistache::Http::Code::Bad_Request,
                    "Could not parse body");
    }

    const auto transaction_bin =
        Buffer(publish_message.transaction(),
               Buffer::InputType::HEX);
    const auto signature = Buffer(publish_message.signature(),
                                  Buffer::InputType::HEX);

    messages::Transaction transaction;
    if (!messages::from_buffer(transaction_bin, &transaction)) {
      response.send(Pistache::Http::Code::Bad_Request,
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
      response.send(Pistache::Http::Code::Bad_Request, "Bad signature");
      return;
    }

    if (!transaction_publish(transaction)) {
      response.send(Pistache::Http::Code::Bad_Request,
                    "Could not publish transaction");
      return;
    }

    response.send(Pistache::Http::Code::Ok);
  }

public:
  Rest(const messages::config::Rest &config,
       Bot *bot) :
      Api::Api(bot),
      httpEndpoint(std::make_shared<Http::Endpoint>(Address (Ipv4::any(), config.port()))) {

    init();
  }

  ~Rest() {
    shutdown();
  }
};

}  // namespace api
}  // namespace neuro

#endif /* NEURO_SRC_API_REST_HPP */


