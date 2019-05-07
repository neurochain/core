#ifndef NEURO_SRC_REST_HPP
#define NEURO_SRC_REST_HPP

#include "common/types.hpp"
#include "api/Api.hpp"

#include <memory>
#include <thread>

namespace neuro {
class Bot;
namespace rest {

class Rest {
 private:
  Api *_api;
  
 public:
  Rest(const messages::config::Rest &config,
       Api *api);

  void join();
  void stop();
  ~Rest() { stop(); }
};

}  // namespace rest
}  // namespace neuro

#endif /* NEURO_SRC_REST_HPP */
