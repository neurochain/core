#ifndef NEURO_SRC_API_API_HPP
#define NEURO_SRC_API_API_HPP

#include "messages/Message.hpp"
#include "messages/Peers.hpp"

namespace neuro {
class Bot;
namespace api {

class Api {
 private:
  Bot *_bot;

 public:
  explicit Api(Bot *bot);
  virtual ~Api() = default;

 protected:
  virtual bool verify_key_pub_format(const messages::_KeyPub &key_pub) final;
  virtual messages::NCCAmount balance(const messages::_KeyPub &key_pub) final;
  virtual std::vector<messages::_KeyPub> previous_recipients(
      const messages::_KeyPub &key_pub) final;

  virtual messages::Block block(messages::BlockID id) final;
  virtual messages::Block block(messages::BlockHeight height) final;
  virtual messages::Blocks last_blocks(std::size_t nb_blocks) final;
  virtual messages::BlockHeight height() const final;

  virtual std::size_t total_nb_transactions() const final;
  virtual std::size_t total_nb_blocks() const final;

  virtual bool is_ledger_uptodate() const final;
  virtual Buffer transaction(
      const messages::Transaction &transaction) const final;
  virtual messages::Transaction build_transaction(
      const messages::_KeyPub &sender_key_pub,
      const google::protobuf::RepeatedPtrField<messages::Output> &outputs,
      const std::optional<messages::NCCAmount> &fees) final;
  virtual std::optional<messages::Transaction> transaction_from_json(
      const std::string &json) final;
  virtual messages::Transaction transaction(
      const messages::TransactionID &id) final;
  virtual messages::Transactions list_transactions(const messages::_KeyPub &key_pub) const;
  virtual bool transaction_publish(
      const messages::Transaction &transaction) final;

  virtual const messages::Peers &peers() const final;
  virtual const messages::_Peers connections() const final;
};

}  // namespace api
}  // namespace neuro

#endif /* NEURO_SRC_API_API_HPP */
