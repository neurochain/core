#include <onion/exportlocal.h>
#include <onion/extrahandlers.hpp>
#include <onion/request.hpp>
#include <onion/response.hpp>

#include "common/logger.hpp"
#include "ledger/Ledger.hpp"
#include "rest/Rest.hpp"

namespace neuro {
namespace rest {

Rest::Rest(std::shared_ptr<ledger::Ledger> ledger,
           std::shared_ptr<networking::Networking> networking,
           std::shared_ptr<crypto::Ecc> keys,
           std::shared_ptr<consensus::Consensus> consensus,
           const messages::config::Rest &config)
    : _ledger(ledger),
      _networking(networking),
      _keys(keys),
      _consensus(consensus),
      _config(config),
      _port(_config.port()),
      _static_path(_config.static_path()),
      _server(O_POOL) {
  _server.setPort(_port);
  _server.setHostname("0.0.0.0");
  _root = std::make_unique<Onion::Url>(&_server);

  const auto list_transactions_route = [this](Onion::Request &req,
                                              Onion::Response &res) {
    const auto address = req.query("address", "");
    LOG_INFO << "ADDRESS " << address;
    res << list_transactions(address);
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

  const auto faucet_send_route = [this](Onion::Request &req,
                                        Onion::Response &res) {
    const auto faucet_amount = _config.faucet_amount();
    const auto address_str = req.query("address", "");
    LOG_INFO << "ADDRESS " << address_str;
    const messages::Address address = load_hash(address_str);
    if (_ledger->has_received_transaction(address)) {
      res << "{\"error\": \"The given address has already received some "
             "coins.\"";
      return OCS_PROCESSED;
    } else {
      messages::Transaction transaction =
          build_faucet_transaction(address, faucet_amount);
      publish_transaction(transaction);
      std::string json;
      messages::to_json(transaction, &json);
      res << json;
      return OCS_PROCESSED;
    }
  };

  const auto get_transaction_route = [this](Onion::Request &req,
                                            Onion::Response &res) {
    const auto transaction_id_str = req.query("transaction_id", "");
    const messages::Hasher transaction_id = load_hash(transaction_id_str);
    messages::Transaction transaction;
    _ledger->get_transaction(transaction_id, &transaction);
    std::string json;
    messages::to_json(transaction, &json);
    res << json;
    return OCS_PROCESSED;
  };

  const auto get_block_route = [this](Onion::Request &req,
                                      Onion::Response &res) {
    const auto block_id_str = req.query("block_id", "");
    const messages::Hasher block_id = load_hash(block_id_str);
    messages::Block block;
    if (block_id_str != "") {
      _ledger->get_block(block_id, &block);
    } else {
      const auto block_height_str = req.query("height", "");
      if (block_height_str != "") {
        int block_height = std::stoi(block_height_str);
        _ledger->get_block(block_height, &block);
      }
    }
    std::string json;
    messages::to_json(block, &json);
    res << json;
    return OCS_PROCESSED;
  };

  const auto last_blocks_route = [this](Onion::Request &req,
                                        Onion::Response &res) {
    const auto nb_blocks_str = req.query("nb_blocks", "10");
    int nb_blocks = std::stoi(nb_blocks_str);
    std::vector<messages::Block> blocks_vector =
        _ledger->get_last_blocks(nb_blocks);
    messages::Blocks blocks;
    for (auto block : blocks_vector) {
      blocks.add_blocks()->CopyFrom(block);
    }
    std::string json;
    messages::to_json(blocks, &json);
    res << json;
    return OCS_PROCESSED;
  };

  _root->add("list_transactions", list_transactions_route);
  _root->add("publish_transaction", publish_transaction_route);
  _root->add("generate_keys", generate_keys_route);
  if (_config.has_faucet_amount()) {
    _root->add("faucet_send", faucet_send_route);
  }
  _root->add("get_transaction", get_transaction_route);
  _root->add("get_block", get_block_route);
  _root->add("get_last_blocks", last_blocks_route);
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

messages::UnspentTransactions Rest::list_unspent_transactions(
    const messages::Address &address) const {
  messages::UnspentTransactions unspent_transactions;

  auto transactions = _ledger->list_transactions(address).transactions();
  for (auto transaction : transactions) {
    for (int i = 0; i < transaction.outputs_size(); i++) {
      auto output = transaction.outputs(i);
      if (output.address() == address &&
          _ledger->is_unspent_output(transaction, i)) {
        auto unspent_transaction =
            unspent_transactions.add_unspent_transactions();
        unspent_transaction->set_transaction_id(transaction.id().data());
        unspent_transaction->set_value(std::to_string(output.value().value()));
      }
    }
  }
  return unspent_transactions;
}

std::string Rest::list_transactions(const std::string &address_str) const {
  const messages::Hasher address = load_hash(address_str);
  messages::UnspentTransactions unspent_transactions =
      list_unspent_transactions(address);
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
      input->set_key_id(0);
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
  // Add the transaction to the transaction pool
  _consensus->add_transaction(transaction);

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

messages::Transaction Rest::build_faucet_transaction(
    const messages::Address &address, const uint64_t amount) {
  // Set transactions_ids
  auto bot_address = messages::Hasher(_keys->public_key());
  messages::UnspentTransactions unspent_transactions =
      list_unspent_transactions(bot_address);
  messages::TransactionToPublish transaction_to_publish;
  for (auto unspent_transaction : unspent_transactions.unspent_transactions()) {
    auto transaction_id = transaction_to_publish.add_transactions_ids();
    *transaction_id = unspent_transaction.transaction_id();
  }

  // Set outputs
  auto output = transaction_to_publish.add_outputs();
  output->mutable_address()->CopyFrom(address);
  output->mutable_value()->set_value(amount);

  // Set key_priv
  messages::KeyPriv key_priv;
  _keys->private_key().save(&key_priv);
  transaction_to_publish.set_key_priv(key_priv.data());

  // Set fees
  transaction_to_publish.mutable_fees()->set_value(0);

  messages::Transaction transaction = build_transaction(transaction_to_publish);
  return transaction;
}

void Rest::join() { _thread.join(); }

void Rest::stop() {
  _server.listenStop();
  _thread.join();
}

}  // namespace rest
}  // namespace neuro
