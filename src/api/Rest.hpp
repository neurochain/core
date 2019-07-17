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

  const std::string B64ALPHABET =
      "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

  std::string encode_base64(const CryptoPP::Integer &num) {
    CryptoPP::Integer padded_num = num << (6 - num.BitCount() % 6);
    std::stringstream encoded_stream;
    CryptoPP::Integer quotient;
    CryptoPP::Integer remainder;
    while (padded_num > 0) {
      CryptoPP::Integer::DivideByPowerOf2(remainder, quotient, padded_num, 6);
      encoded_stream << B64ALPHABET[remainder.ConvertToLong()];
      padded_num = quotient;
    }
    std::string encoded = encoded_stream.str();
    std::reverse(encoded.begin(), encoded.end());
    encoded += std::string(4 - encoded.size() % 4, '=');
    return encoded;
  }

  using Request = Pistache::Rest::Request;
  using Response = Pistache::Http::ResponseWriter;
   //  Api *_api;
  std::shared_ptr<Http::Endpoint> _httpEndpoint;
  Pistache::Rest::Router _router;

  void init();
  void start();
  void shutdown();
  template<class T>
  std::string to_json(const google::protobuf::RepeatedPtrField<T>& fields);
  std::string to_json(const neuro::messages::Hash &hash);
  void send(Response &response, const messages::Packet &packet);
  void send(Response &response, const messages::Transaction &transaction);
  void send(Response &response, const messages::Block &block);
  void send(Response &response, const std::string &value);
  void bad_request(Response &response, const std::string& message);
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

  std::string to_internal_id(const std::string &id);
public:
  Rest(const messages::config::Rest &config, Bot *bot);
  ~Rest();
};

}  // namespace api
}  // namespace neuro

#endif /* NEURO_SRC_API_REST_HPP */


