#ifndef NEURO_SRC_LEDGER_WALLET_HPP
#define NEURO_SRC_LEDGER_WALLET_HPP

namespace neuro {
namespace networking {
class Networking;
}  // namespace networking
namespace ledger {

class Wallet {
 protected:
  std::shared_ptr<Ledger> _ledger;
  std::shared_ptr<networking::Networking> _networking;
  const crypto::Ecc _keys;

 public:
  Wallet(std::shared_ptr<Ledger> ledger,
         std::shared_ptr<networking::Networking> networking,
         const std::string &path_key_priv, const std::string &path_key_pub)
      : _ledger(ledger),
        _networking(networking),
        _keys(path_key_priv, path_key_pub) {}

  messages::UnspentTransactions unspent_transactions() const {
    const auto address = messages::Hasher(_keys.public_key());
    return _ledger->unspent_transactions(address);
  }

  void publish_transaction(messages::Transaction &transaction) const {
    _consensus->add_transaction(transaction);

    auto message = std::make_shared<messages::Message>();
    messages::fill_header(message->mutable_header());
    auto body = message->add_bodies();
    body->mutable_transaction()->CopyFrom(transaction);
    _networking->send(message, networking::ProtocolType::PROTOBUF2);
  }

  virtual ~Wallet() {}
};

}  // namespace ledger
}  // namespace neuro

#endif /* NEURO_SRC_LEDGER_WALLET_HPP */
