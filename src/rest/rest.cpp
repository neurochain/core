#include <onion/onion.hpp>
#include <onion/request.hpp>
#include <onion/response.hpp>
#include <onion/url.hpp>
#include "crypto/Ecc.hpp"
#include "crypto/Hash.hpp"
#include "ledger/LedgerMongodb.hpp"
#include "messages.pb.h"
#include "messages/Hasher.hpp"
#include "messages/Message.hpp"

namespace neuro {
namespace rest {

  std::string get_transactions(ledger::LedgerMongodb &ledger,
			       const std::string &address) {
  messages::Hasher addr;
  addr.set_type(messages::Hash::SHA256);
  addr.set_data(address.c_str(), address.size());
  ledger::Ledger::Filter filter;
  filter.output_key_id(addr);
  std::vector<messages::Transaction> transactions;

  ledger.for_each(filter, [&](const messages::Transaction &a) {
    transactions.push_back(std::move(a));
    return true;
  });

  // for_each(transactions.begin(), polygon.end(), [&sum](int i) { sum += i; });
  // boost::algorithm::join(elems, ", ");

  std::string t;
  messages::to_json(transactions.front(), &t);
  return t;
}

int main(int argc, char **argv) {
  ONION_INFO("Listening at http://localhost:8080");
  Onion::Onion server(O_POOL);
  Onion::Url root(&server);
  // auto ledger = LedgerMongodb("", "");
  auto ledger =
      ledger::LedgerMongodb("mongodb://127.0.0.1:27017/neuro", "neuro");

  const auto list_transactions = [&ledger](Onion::Request &req,
                                           Onion::Response &res) {
    res << "list transactions ?";
    const auto toto = req.query("toto", "nothing there");
    res << get_transactions(ledger,
                            "/yYWmEyqfHRJIrrOrxcRL+TiDcIAhjr7XQQ+EhNPGsc=");
    return OCS_PROCESSED;
  };

  const auto publish_transaction = [&ledger](Onion::Request &req,
                                             Onion::Response &res) {
    res << "publish_transactions";
    const auto inputs = req.post("inputs", "");
    const auto outputs = req.post("outputs", "");
    const auto fee = req.post("fee", "");
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
