#include <onion/request.hpp>
#include <onion/response.hpp>
#include "ledger/Ledger.hpp"

#include "common/logger.hpp"
#include "rest/Rest.hpp"

/*
 * TODO generate_keys
 * publish transaction
 * txids
 * get outputs
 * --> inputs
 * outputs from Lev
 * then sign
 * then fees
 */

namespace neuro {
namespace rest {

messages::Hasher load_hash(const std::string &hash_str) {
  messages::Hasher result;
  std::stringstream hash_json;
  hash_json << "{\"type\": \"SHA256\", \"data\": \"" << hash_str << "\"}";
  messages::from_json(hash_json.str(), &result);
  return result;
}

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
    messages::TransactionToPublish transaction_to_publish;
    messages::from_json(post_data, &transaction_to_publish);
    messages::Transaction transaction;

    // Load private key
    auto buffer = Buffer(transaction_to_publish.key_priv());
    const auto random_pool = std::make_shared<CryptoPP::AutoSeededRandomPool>();
    auto key_priv = crypto::EccPriv(random_pool);
    key_priv.load(buffer);
    const crypto::EccPub key_pub = key_priv.make_public_key();
    const auto address = messages::Address(key_pub);

    // Process the outputs and lookup their output_id to build the inputs
    for (auto transaction_id_str : transaction_to_publish.transactions_ids()) {
      auto transaction_id = load_hash(transaction_id_str);
      auto outputs = _ledger->get_outputs_for_address(transaction_id, address);
      for (auto output : outputs) {
        auto input = transaction.add_inputs();
        input->mutable_id()->CopyFrom(transaction.id());
        input->set_output_id(output.output_id());
      }
    }
    res << "publish_transactions";
    return OCS_PROCESSED;
  };

  _root.add("list_transactions", list_transactions);
  _root.add("publish_transaction", publish_transaction);
  _thread = std::thread([this]() { _server.listen(); });
}

std::string Rest::get_address_transactions(
    std::shared_ptr<ledger::Ledger> ledger,
    const std::string &address_str) const {
  auto buffer = Buffer(address_str);

  messages::UnspentTransactions unspent_transactions;
  messages::Hasher address = load_hash(address_str);
  auto transactions = ledger->list_transactions(address).transactions();
  for (auto transaction : transactions) {
    for (int i = 0; i < transaction.outputs_size(); ++i) {
      auto output = transaction.outputs(i);
      if (ledger->is_unspent_output(transaction, i) &&
          output.address() == address) {
        auto unspent_transaction =
            unspent_transactions.add_unspent_transactions();
        unspent_transaction->set_transaction_id(output.address().data());
        unspent_transaction->set_value(output.value().value());
      }
    }
  }
  std::string result;
  messages::to_json(unspent_transactions, &result);
  return result;
}

void Rest::join() { _thread.join(); }

void Rest::stop() {
  _server.listenStop();
  _thread.join();
}

}  // namespace rest
}  // namespace neuro
