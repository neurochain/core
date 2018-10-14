#include <onion/exportlocal.h>
#include <onion/extrahandlers.hpp>
#include <onion/request.hpp>
#include <onion/response.hpp>
#include "ledger/Ledger.hpp"

#include "common/logger.hpp"
#include "rest/Rest.hpp"

/*
 * TODO
 * send transaction _networking->send(transaction, ProtocolType::PROTOBUF2)
 * get transaction by id
 * remove spent transactions
 */

namespace neuro {
namespace rest {

Rest::Rest(std::shared_ptr<ledger::Ledger> ledger,
           std::shared_ptr<networking::Networking> networking,
           const messages::config::Rest &config)
    : _ledger(ledger),
      _networking(networking),
      _config(config),
      _port(_config.port()),
      _static_path(_config.static_path()),
      _server(O_POOL) {
  LOG_INFO << "Listening at http://127.0.0.1:" << _port;
  _server.setPort(_port);
  _server.setHostname("127.0.0.1");
  _root = std::make_unique<Onion::Url>(&_server);
  const auto list_transactions_route = [this](Onion::Request &req,
                                              Onion::Response &res) {
    const auto address = req.query("address", "");
    LOG_INFO << "ADDRESS " << address;
    res << list_transactions(_ledger, address);
    return OCS_PROCESSED;
  };

  const auto publish_transaction_route = [this](Onion::Request &req,
                                                Onion::Response &res) {
    onion_request *c_req = req.c_handler();
    const onion_block *dreq = onion_request_get_data(c_req);
    std::string post_data = onion_block_data(dreq);
    messages::TransactionToPublish transaction_to_publish;
    messages::from_json(post_data, &transaction_to_publish);
    auto transaction = build_transaction(transaction_to_publish);
    publish_transaction(transaction);
    std::string json;
    messages::to_json(transaction, &json);
    res << json;
    return OCS_PROCESSED;
  };

  const auto generate_keys_route = [this](Onion::Request &req,
                                          Onion::Response &res) {
    messages::GeneratedKeys generated_keys = generate_keys();
    std::string json;
    messages::to_json(generated_keys, &json);
    res << json;
    return OCS_PROCESSED;
  };

  _root->add("list_transactions", list_transactions_route);
  _root->add("publish_transaction", publish_transaction_route);
  _root->add("generate_keys", generate_keys_route);
  serve_folder("^static/", "static");
  serve_file("", "index.html");
  serve_file("index.html");
  serve_file("asset-manifest.json");
  serve_file("favicon.ico");
  serve_file("manifest.json");
  serve_file("service-worker.js");
  _thread = std::thread([this]() { _server.listen(); });
}

void Rest::serve_file(const std::string filename) {
  _root->add(filename,
             Onion::Shortcuts::static_file((_static_path + filename).c_str()));
}

void Rest::serve_file(const std::string route, const std::string filename) {
  _root->add(route,
             Onion::Shortcuts::static_file((_static_path + filename).c_str()));
}

void Rest::serve_folder(const std::string route, const std::string foldername) {
  _root->add(route, onion_handler_export_local_new(
                        (_static_path + foldername).c_str()));
}

messages::Hasher Rest::load_hash(const std::string &hash_str) const {
  messages::Hasher result;
  std::stringstream hash_json;
  hash_json << "{\"type\": \"SHA256\", \"data\": \"" << hash_str << "\"}";
  messages::from_json(hash_json.str(), &result);
  return result;
}

std::string Rest::list_transactions(std::shared_ptr<ledger::Ledger> ledger,
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
        unspent_transaction->set_transaction_id(transaction.id().data());
        unspent_transaction->set_value(std::to_string(output.value().value()));
      }
    }
  }
  std::string result;
  messages::to_json(unspent_transactions, &result);
  return result;
}

messages::Transaction Rest::build_transaction(
    const messages::TransactionToPublish &transaction_to_publish) const {
  messages::Transaction transaction;

  // Load keys
  auto buffer = Buffer(transaction_to_publish.key_priv());
  const auto random_pool = std::make_shared<CryptoPP::AutoSeededRandomPool>();
  auto key_priv = crypto::EccPriv(random_pool);
  key_priv.load(buffer);
  const crypto::EccPub key_pub = key_priv.make_public_key();
  const auto address = messages::Address(key_pub);
  const auto ecc = crypto::Ecc(key_priv, key_pub);
  std::vector<const crypto::Ecc *> keys = {&ecc};

  uint64_t inputs_ncc = 0;
  uint64_t outputs_ncc = 0;
  auto outputs_to_publish = transaction_to_publish.outputs();
  for (auto output : outputs_to_publish) {
    outputs_ncc += output.value().value();
  }

  // Process the outputs and lookup their output_id to build the inputs
  auto transaction_ids = transaction_to_publish.transactions_ids();
  for (auto transaction_id_str : transaction_ids) {
    messages::Hasher transaction_id;
    transaction_id.set_type(messages::Hash_Type_SHA256);
    transaction_id.set_data(transaction_id_str);
    auto outputs = _ledger->get_outputs_for_address(transaction_id, address);
    for (auto output : outputs) {
      auto input = transaction.add_inputs();
      input->mutable_id()->CopyFrom(transaction_id);
      input->set_output_id(output.output_id());
      inputs_ncc += output.value().value();
    }
  }

  transaction.mutable_outputs()->CopyFrom(transaction_to_publish.outputs());

  // Add change output
  if (outputs_ncc < inputs_ncc) {
    auto change = transaction.add_outputs();
    change->mutable_value()->set_value(inputs_ncc - outputs_ncc);
    change->mutable_address()->CopyFrom(address);
  }

  transaction.mutable_fees()->CopyFrom(transaction_to_publish.fees());

  // Sign the transaction
  crypto::sign(keys, &transaction);

  // Hash the transaction
  messages::hash_transaction(&transaction);

  return transaction;
}

void Rest::publish_transaction(messages::Transaction &transaction) const {
  // TODO Add the transaction to the transaction pool

  // Send the transaction on the network
  auto message = std::make_shared<messages::Message>();
  messages::fill_header(message->mutable_header());
  auto body = message->add_bodies();
  body->mutable_transaction()->CopyFrom(transaction);
  _networking->send(message, networking::ProtocolType::PROTOBUF2);
}

messages::GeneratedKeys Rest::generate_keys() const {
  messages::GeneratedKeys generated_keys;
  crypto::Ecc ecc;
  messages::KeyPub key_pub;
  ecc.public_key().save(&key_pub);
  messages::KeyPriv key_priv;
  ecc.private_key().save(&key_priv);
  generated_keys.mutable_key_priv()->CopyFrom(key_priv);
  generated_keys.mutable_key_pub()->CopyFrom(key_pub);
  generated_keys.mutable_address()->CopyFrom(
      messages::Hasher(ecc.public_key()));
  return generated_keys;
}

void Rest::join() { _thread.join(); }

void Rest::stop() {
  _server.listenStop();
  _thread.join();
}

}  // namespace rest
}  // namespace neuro
