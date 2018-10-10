#ifndef NEURO_SRC_REST_HPP
#define NEURO_SRC_REST_HPP

#include <thread>
#include "common/types.hpp"

namespace neuro {
namespace rest {

class Rest {
 private:
  const Port _port;
  std::shared_ptr<ledger::Ledger> _ledger;

  Onion::Onion _server;
  Onion::Url _root;

  std::thread _thread;

 public:
  Rest(const Port port,
       std::shared_ptr<ledger::Ledger> ledger);

  void stop();
};


}  // rest
}  // neuro

#endif /* NEURO_SRC_REST_HPP */
