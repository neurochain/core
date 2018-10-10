#include <onion/request.hpp>
#include <onion/response.hpp>
#include "ledger/Ledger.hpp"

#include "common/logger.hpp"
#include "rest/Rest.hpp"

namespace neuro {
namespace rest {

Rest::Rest(const Port port, std::shared_ptr<ledger::Ledger> ledger)
    : _port(port), _ledger(ledger), _server(O_POOL), _root(&_server) {
  LOG_INFO << "Listening at http://localhost:" << port;

  const auto list_transactions = [this](Onion::Request &req,
                                        Onion::Response &res) {
    const auto address = req.query("address", "");
    LOG_INFO << "ADDRESS " << address;
    res << get_address_transactions(_ledger, address);
    return OCS_PROCESSED;
  };

  const auto publish_transaction = [this](Onion::Request &req,
                                          Onion::Response &res) {
    onion_request *c_req = req.c_handler();
    const onion_block *dreq = onion_request_get_data(c_req);
    std::string post_data = onion_block_data(dreq);
    messages::Transaction transaction;
    messages::from_json(post_data, &transaction);
    res << "publish_transactions";
    return OCS_PROCESSED;
  };

  _root.add("list_transactions", list_transactions);
  _root.add("publish_transaction", publish_transaction);
  _thread = std::thread([this]() { _server.listen(); });
}

std::string Rest::get_address_transactions(
    std::shared_ptr<ledger::Ledger> ledger, const std::string &address) const {
  auto buffer = Buffer(address);
  auto addr = messages::Hasher(buffer);
  std::string t;
  messages::to_json(ledger->list_transactions(addr), &t);
  return t;
}

void Rest::stop() {
  _server.listenStop();
  _thread.join();
}

}  // namespace rest
}  // namespace neuro
