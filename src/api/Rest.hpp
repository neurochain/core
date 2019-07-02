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
  using Request = Pistache::Rest::Request;
  using Response = Pistache::Http::ResponseWriter;
   //  Api *_api;
  std::shared_ptr<Http::Endpoint> _httpEndpoint;
  Pistache::Rest::Router _router;

  void init();
  void start();
  void shutdown();
  void setupRoutes();

  void get_balance(const Request& request, Response response);
  void get_ready(const Request& request, Response response);
  void get_transaction(const Request& request, Response response);
  void get_create_transaction (const Request& request, Response response);
  void publish(const Request &request, Response response);
  void get_unspent_transaction_list(const Request &request, Response response);
  void get_block_by_id(const Request& request, Response response);
  void get_block_by_height(const Request& request, Response response);
  void get_last_blocks(const Request& request, Response response);
  void get_total_nb_transactions(const Request& request, Response response);
  void get_total_nb_blocks(const Request& request, Response response);
  void get_peers(const Request& request, Response response);

public:
  Rest(const messages::config::Rest &config, Bot *bot);
  ~Rest();
};

}  // namespace api
}  // namespace neuro

#endif /* NEURO_SRC_API_REST_HPP */


