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

// class StatsEndpoint {
// public:
//   StatsEndpoint(Address addr)
//     : 
//   { }

//   void init() {
//     auto opts = Http::Endpoint::options()
//       .threads(1)
//       .flags(Tcp::Options::InstallSignalHandler);
//     httpEndpoint->init(opts);
//     setupRoutes();
//   }

// private:
//   void setupRoutes() {
//     using namespace Rest;

//     Routes::Post(router, "/record/:name/:value?", Routes::bind(&StatsEndpoint::doRecordMetric, this));
//     Routes::Get(router, "/value/:name", Routes::bind(&StatsEndpoint::doGetMetric, this));
//     Routes::Get(router, "/auth", Routes::bind(&StatsEndpoint::doAuth, this));

//   }

//   void doRecordMetric(const Rest::Request& request, Http::ResponseWriter response) {
//     auto name = request.param(":name").as<std::string>();

//     Guard guard(metricsLock);
//     auto it = std::find_if(metrics.begin(), metrics.end(), [&](const Metric& metric) {
// 	return metric.name() == name;
//       });

//     int val = 1;
//     if (request.hasParam(":value")) {
//       auto value = request.param(":value");
//       val = value.as<int>();
//     }

//     if (it == std::end(metrics)) {
//       metrics.push_back(Metric(std::move(name), val));
//       response.send(Http::Code::Created, std::to_string(val));
//     }
//     else {
//       auto &metric = *it;
//       metric.incr(val);
//       response.send(Http::Code::Ok, std::to_string(metric.value()));
//     }

//   }

//   void doGetMetric(const Rest::Request& request, Http::ResponseWriter response) {
//     auto name = request.param(":name").as<std::string>();

//     Guard guard(metricsLock);
//     auto it = std::find_if(metrics.begin(), metrics.end(), [&](const Metric& metric) {
// 	return metric.name() == name;
//       });

//     if (it == std::end(metrics)) {
//       response.send(Http::Code::Not_Found, "Metric does not exist");
//     } else {
//       const auto& metric = *it;
//       response.send(Http::Code::Ok, std::to_string(metric.value()));
//     }
//   }

//   void doAuth(const Rest::Request& request, Http::ResponseWriter response) {
//     //printCookies(request);
//     response.cookies()
//       .add(Http::Cookie("lang", "en-US"));
//     response.send(Http::Code::Ok);
//   }

//   class Metric {
//   public:
//     Metric(std::string name, int initialValue = 1)
//       : name_(std::move(name))
//       , value_(initialValue)
//     { }

//     int incr(int n = 1) {
//       int old = value_;
//       value_ += n;
//       return old;
//     }

//     int value() const {
//       return value_;
//     }

//     std::string name() const {
//       return name_;
//     }
//   private:
//     std::string name_;
//     int value_;
//   };

//   typedef std::mutex Lock;
//   typedef std::lock_guard<Lock> Guard;
//   Lock metricsLock;
//   std::vector<Metric> metrics;

// };

  
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
    // Routes::Get(router, "/value/:name", Routes::bind(&StatsEndpoint::doGetMetric, this));
    // Routes::Get(router, "/auth", Routes::bind(&StatsEndpoint::doAuth, this));

  }

 public:
  Rest(const messages::config::Rest &config,
       Bot *bot) :
      Api::Api(bot),
      httpEndpoint(std::make_shared<Http::Endpoint>(Address (Ipv4::any(), config.port()))) {

    init();
    start();
  }

};

}  // namespace api
}  // namespace neuro

#endif /* NEURO_SRC_API_REST_HPP */


