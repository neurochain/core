#include "rest/Rest.hpp"

namespace neuro {
namespace rest {

Rest::Rest(const Port port,
           std::shared_ptr<ledger::Ledger> ledger):
    _port(port),
    _leger(ledger),
    _server(O_POOL),
    _root(&server) {
  
  ONION_INFO("Listening at http://localhost:" + std::to_string(port));

  const auto list_transactions = [&_ledger](Onion::Request &req,
                                            Onion::Response &res) {
    const auto address = req.query("address", "");
    ONION_INFO("ADDRESS " + address);
    res << get_address_transactions(ledger, address);
    return OCS_PROCESSED;
  };

  const auto publish_transaction = [&ledger](Onion::Request &req,
                                             Onion::Response &res) {
    onion_request *c_req = req.c_handler();
    const onion_block *dreq = onion_request_get_data(c_req);
    std::string post_data = onion_block_data(dreq);
    messages::Transaction transaction;
    messages::from_json(post_data, &transaction);
    res << "publish_transactions";
    return OCS_PROCESSED;
  };

  root.add("list_transactions", list_transactions);
  root.add("publish_transaction", publish_transaction);
  _server = std::thread([&server]server.listen());
}

void Rest::stop() {
  _server.listenStop();
  _thread.join();
}


}  // rest
}  // neuro
