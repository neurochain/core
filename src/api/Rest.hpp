#ifndef NEURO_SRC_API_REST_HPP
#define NEURO_SRC_API_REST_HPP

#include "common/types.hpp"
#include "api/Api.hpp"

#include <memory>
#include <thread>
#include <pistache/http.h>
#include <pistache/router.h>
#include <pistache/endpoint.h>

using namespace Pistache;

namespace neuro {
class Bot;
namespace api {

class Rest : public Api {
 private:
  Api *_api;
  std::shared_ptr<Http::Endpoint> httpEndpoint;
  Pistache::Rest::Router router;

  void init() {
    auto opts = Http::Endpoint::options()
                .threads(1)
                .flags(Tcp::Options::InstallSignalHandler);
    httpEndpoint->init(opts);
    setupRoutes();
    start();
  }

  void start() {
    httpEndpoint->setHandler(router.handler());
    httpEndpoint->serve();
  }

  void shutdown() {
    httpEndpoint->shutdown();
  }
  
  void setupRoutes() {
    using namespace Rest;

    // Routes::Post(router, "/record/:name/:value?", Routes::bind(&StatsEndpoint::doRecordMetric, this));
    Routes::Get(router, "/balance/:address", Routes::bind(&Rest::get_balance, this));
    Routes::Get(router, "/ready", Routes::bind(&Rest::get_ready, this));
    Routes::Get(router, "/fill_transaction/:address/:fees", Routes::bind(&Rest::get_create_transaction, this));

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
      response.send(Pistache::Http::Code::Bad_Request, "Could not set inputs");
      return;
    }
    
    response.send(Pistache::Http::Code::Ok, messages::to_json(transaction));
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


