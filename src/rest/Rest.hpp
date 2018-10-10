#ifndef NEURO_SRC_REST_HPP
#define NEURO_SRC_REST_HPP

#include "common/types.hpp"

#include <thread>

#include <onion/onion.hpp>
#include <onion/url.hpp>

namespace neuro {
namespace rest {

class Rest {
 private:
  const Port _port;
  std::shared_ptr<ledger::Ledger> _ledger;

  Onion::Onion _server;
  Onion::Url _root;

  std::thread _thread;
  std::string get_address_transactions(std::shared_ptr<ledger::Ledger> ledger,
                                       const std::string &address) const;

 public:
  Rest(const Port port, std::shared_ptr<ledger::Ledger> ledger);

  void stop();
};

}  // namespace rest
}  // namespace neuro

#endif /* NEURO_SRC_REST_HPP */
