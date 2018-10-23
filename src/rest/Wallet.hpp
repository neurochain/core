#ifndef NEURO_SRC_REST_WALLET_HPP
#define NEURO_SRC_REST_WALLET_HPP

#include "ledger/Wallet.hpp"
#include "rest/Rest.hpp"

namespace neuro {
namespace ledger {
class Ledger;
}  // namespace ledger
namespace rest {

class Wallet : public ledger::Wallet {
 private:
  rest::Rest _rest;

 public:
  Wallet(std::shared_ptr<ledger::Ledger> ledger,
         std::shared_ptr<networking::Networking> networking,
         const std::string &path_key_priv, const std::string &path_key_pub,
         const std::string &endpoint, const Port port)
      : ledger::Wallet::Wallet(ledger, networking, path_key_priv, path_key_pub),
        _rest(endpoint, port) {}

  Rest::Status list_transactions(Rest::Request &req, Rest::Response &res) {
    const auto address_str = req.query("address", "");
    LOG_INFO << "ADDRESS " << address_str;
    const auto address = messages::Hasher{address_str};

    const auto transactions = _ledger->unspent_transactions(address);
    std::string result;
    messages::to_json(transactions, &result);
    res << result;
    return OCS_PROCESSED;
  }

  Rest::Status publish_transaction(Rest::Request &req, Rest::Response &res) {
    onion_request *c_req = req.c_handler();
    const onion_block *dreq = onion_request_get_data(c_req);
    const std::string &post_data = onion_block_data(dreq);
    messages::TransactionToPublish transaction_to_publish;
    messages::from_json(post_data, &transaction_to_publish);
    auto transaction = _ledger->build_transaction(transaction_to_publish);
    publish_transaction(transaction);
    std::string json;
    messages::to_json(transaction, &json);
    res << json;

    return OCS_PROCESSED;
  }

  Rest::Status generate_keys(Rest::Request &req, Rest::Response &res) {
    messages::GeneratedKeys generated_keys = generate_keys();
    std::string json;
    messages::to_json(generated_keys, &json);
    res << json;
    return OCS_PROCESSED;
  }

  Rest::Status get_transaction(Onion::Request &req, Onion::Response &res) {
    const auto transaction_id_str = req.query("transaction_id", "");
    const messages::Hasher transaction_id = load_hash(transaction_id_str);
    messages::Transaction transaction;
    _ledger->get_transaction(transaction_id, &transaction);
    std::string json;
    messages::to_json(transaction, &json);
    res << json;
    return OCS_PROCESSED;
  }
};

}  // namespace rest
}  // namespace neuro

#endif /* NEURO_SRC_REST_WALLET_HPP */
