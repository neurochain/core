#ifndef NEURO_SRC_REST_HPP
#define NEURO_SRC_REST_HPP

#include "common/types.hpp"
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
  messages::config::Rest _config;
  Port _port;
  std::string _static_path;

  Onion::Onion _server;
  std::unique_ptr<Onion::Url> _root;

  std::thread _thread;
  std::string list_transactions(const std::string &address) const;
  messages::Transaction build_transaction(
      const messages::TransactionToPublish &transaction_to_publish) const;
  messages::GeneratedKeys generate_keys() const;
  messages::Transaction build_faucet_transaction(
      const messages::Address &address, const uint64_t amount);
  void serve_file(const std::string &filename);
  void serve_file(const std::string &route, const std::string &filename);
  void serve_folder(const std::string &route, const std::string &foldername);

 public:
  Rest(Bot *bot, std::shared_ptr<ledger::Ledger> ledger,
       std::shared_ptr<crypto::Ecc> keys, const messages::config::Rest &config);

  void join();
  void stop();
  ~Rest() { stop(); }
};

}  // namespace rest
}  // namespace neuro

#endif /* NEURO_SRC_REST_HPP */
