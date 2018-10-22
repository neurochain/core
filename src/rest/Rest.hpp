#ifndef NEURO_SRC_REST_REST_HPP
#define NEURO_SRC_REST_REST_HPP

#include <onion/exportlocal.h>
#include <memory>
#include <onion/extrahandlers.hpp>
#include <onion/onion.hpp>
#include <onion/request.hpp>
#include <onion/response.hpp>
#include <onion/url.hpp>
#include <thread>

#include "common/types.hpp"

namespace neuro {
namespace rest {

class Rest {
 public:
  using Handler = Onion::Handler;
  using Status = onion_connection_status;
  using Request = Onion::Request;
  using Response = Onion::Response;

 private:
  Onion::Onion _server;
  std::unique_ptr<Onion::Url> _root;

  std::thread _thread;

 public:
  Rest(const std::string &endpoint, const Port port);

  void run_async();
  void stop();

  void add_endpoint(const std::string &endpoint, Handler &&handler);

  void add_endpoint(const std::string &endpoint, const std::string &file_path);

  virtual ~Rest();
};

}  // namespace rest
}  // namespace neuro

#endif /* NEURO_SRC_REST_REST_HPP */
