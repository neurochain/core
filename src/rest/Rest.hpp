#ifndef NEURO_SRC_REST_HPP
#define NEURO_SRC_REST_HPP

#include "common/types.hpp"
#include "crypto/Sign.hpp"
#include "ledger/Ledger.hpp"
#include "networking/Networking.hpp"

#include <memory>
#include <thread>

#include <onion/onion.hpp>
#include <onion/url.hpp>

namespace neuro {
namespace rest {

class Rest {
 private:
  const Port _port;
  std::shared_ptr<ledger::Ledger> _ledger;
  std::shared_ptr<networking::Networking> _networking;
  messages::config::Config _config;

  Onion::Onion _server;
  std::unique_ptr<Onion::Url> _root;

  std::thread _thread;
  std::string get_address_transactions(std::shared_ptr<ledger::Ledger> ledger,
                                       const std::string &address) const;
  messages::Transaction build_transaction(
      const messages::TransactionToPublish &transaction_to_publish) const;
  void publish_transaction(messages::Transaction &transaction) const;
  messages::Hasher load_hash(const std::string &hash_str) const;
  messages::GeneratedKeys generate_keys() const;

 public:
  Rest(const Port port, std::shared_ptr<ledger::Ledger> ledger,
       std::shared_ptr<networking::Networking> networking,
       messages::config::Config &config);

  void join();
  void stop();
  ~Rest() { stop(); }
};

}  // namespace rest
}  // namespace neuro

#endif /* NEURO_SRC_REST_HPP */
