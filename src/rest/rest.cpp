#include <onion/onion.hpp>
#include <onion/request.hpp>
#include <onion/response.hpp>
#include <onion/url.hpp>
#include "common/Buffer.hpp"
#include "crypto/Ecc.hpp"
#include "crypto/Hash.hpp"
#include "ledger/LedgerMongodb.hpp"
#include "messages.pb.h"
#include "messages/Hasher.hpp"
#include "messages/Message.hpp"

/*
 * TODO generate_keys
 */

namespace neuro {
namespace rest {

std::string get_address_transactions(ledger::LedgerMongodb &ledger,
                                     const std::string &address) {
  auto buffer = Buffer(address);
  auto addr = messages::Hasher(buffer);
  std::string t;
  messages::to_json(ledger.list_transactions(addr), &t);
  return t;
}

int main(int argc, char **argv) {
  ONION_INFO("Listening at http://localhost:8080");
  Onion::Onion server(O_POOL);
  Onion::Url root(&server);
  auto ledger =
      ledger::LedgerMongodb("mongodb://127.0.0.1:27017/neuro", "neuro");

  const auto list_transactions = [&ledger](Onion::Request &req,
                                           Onion::Response &res) {
    const auto address = req.query("address", "");
    ONION_INFO("ADDRESS");
    ONION_INFO(address.c_str());
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
  server.listen();
  return 0;
}

}  // namespace rest
}  // namespace neuro

int main(int argc, char **argv) { neuro::rest::main(argc, argv); }
