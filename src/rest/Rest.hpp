#ifndef NEURO_SRC_REST_HPP
#define NEURO_SRC_REST_HPP

#include "common/types.hpp"
#include "consensus/Consensus.hpp"
#include "consensus/TransactionPool.hpp"
#include "crypto/Sign.hpp"
#include "ledger/Ledger.hpp"

#include <memory>
#include <thread>

#include <onion/onion.hpp>
#include <onion/url.hpp>

namespace neuro {
class Bot;
namespace rest {

class Rest {
 private:
  Bot *_bot;
  std::shared_ptr<ledger::Ledger> _ledger;
  std::shared_ptr<crypto::Ecc> _keys;
  std::shared_ptr<consensus::Consensus> _consensus;
  messages::config::Rest _config;
  Port _port;
  std::string _static_path;

  Onion::Onion _server;
  std::unique_ptr<Onion::Url> _root;

  std::thread _thread;
  std::string list_transactions(const std::string &address) const;
  messages::Transaction build_transaction(
      const messages::TransactionToPublish &transaction_to_publish) const;
  messages::Hasher load_hash(const std::string &hash_str) const;
  messages::GeneratedKeys generate_keys() const;
  messages::Transaction build_faucet_transaction(
      const messages::Address &address, const uint64_t amount);
  void serve_file(const std::string &filename);
  void serve_file(const std::string &route, const std::string &filename);
  void serve_folder(const std::string &route, const std::string &foldername);

  /*  Add for wallet Api */
  void add_cors(Onion::Response &response);
  bool get_post(Onion::Request &request, messages::Packet *packet);
  messages::Hasher pubkey_addr(messages::PublicKey &pubkey);
  std::string pubkey_transactions(messages::PublicKey &pubkey);
  std::string populate_transaction(
      messages::BuildTransaction &buildtransaction);

 public:
  Rest(Bot *bot, std::shared_ptr<ledger::Ledger> ledger,
       std::shared_ptr<crypto::Ecc> keys,
       std::shared_ptr<consensus::Consensus> consensus,
       const messages::config::Rest &config);

  void join();
  void stop();
  ~Rest() { stop(); }
};

}  // namespace rest
}  // namespace neuro

#endif /* NEURO_SRC_REST_HPP */
