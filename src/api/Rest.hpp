#ifndef NEURO_SRC_API_REST_HPP
#define NEURO_SRC_API_REST_HPP

#include "api/Api.hpp"
#include "api/Monitoring.hpp"
#include "common/types.hpp"

#include <pistache/endpoint.h>
#include <pistache/http.h>
#include <pistache/router.h>
#include <memory>
#include <thread>

using namespace Pistache;

namespace neuro {
class Bot;
namespace api {

namespace test {
class Rest;
}

class Rest : public Api {
 private:
  using Request = Pistache::Rest::Request;
  using Response = Pistache::Http::ResponseWriter;
  //  Api *_api;
  std::shared_ptr<Http::Endpoint> _httpEndpoint;
  Pistache::Rest::Router _router;
  Monitoring _monitor;
  static auto constexpr _transaction_per_page = 10;

  void init();
  void start();
  void shutdown();

  std::string raw_data_to_json(std::string raw_data) const;

  void send(Response &response, const messages::Packet &packet);
  void send(Response &response, const std::string &value);
  void bad_request(Response &response, const std::string &message);
  void setupRoutes();

  void get_balance(const Request &request, Response response);
  void post_validate_key(const Request &req, Response res);
  void post_balance(const Request &request, Response response);
  void get_ready(const Request &request, Response response);
  void get_list_transactions(const Request &request, Response response);
  void get_transaction(const Request &request, Response response);
  void get_create_transaction(const Request &request, Response response);
  void publish(const Request &request, Response response);
  void get_block_by_id(const Request &request, Response response);
  void get_block_by_height(const Request &request, Response response);
  void get_last_blocks(const Request &request, Response response);
  void get_total_nb_transactions(const Request &request, Response response);
  void get_total_nb_blocks(const Request &request, Response response);
  void get_peers(const Request &request, Response response);
  void get_connections(const Request &request, Response response);
  void allow_option(const Request &request, Response response);

  void get_status(const Request &request, Response response);
  void get_all_status(const Request &request, Response response);

 public:
  Rest(const messages::config::Rest &config, Bot *bot);
  ~Rest();

  friend class test::Rest;
};

}  // namespace api
}  // namespace neuro

#endif /* NEURO_SRC_API_REST_HPP */
