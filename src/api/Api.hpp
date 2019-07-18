#ifndef NEURO_SRC_API_API_HPP
#define NEURO_SRC_API_API_HPP

#include "messages/Message.hpp"
#include "messages/Address.hpp"
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
  
  virtual bool verify_address_format(const messages::Address &address) final;
  virtual messages::NCCAmount balance(const messages::Address &address) final;
  virtual messages::Addresses previous_recipients(const messages::Address &address) final;

  virtual messages::Block block(messages::BlockID id) final;
  virtual messages::Block block(messages::BlockHeight height) final;
  virtual messages::Blocks last_blocks(std::size_t nb_blocks) final;
  virtual messages::BlockHeight height() const final;

  virtual std::size_t total_nb_transactions() const final;
  virtual std::size_t total_nb_blocks() const final;

  virtual bool is_ledger_uptodate() const final;
  virtual Buffer transaction(const messages::Transaction &transaction) const final;
  virtual std::optional<messages::Transaction> transaction_from_json(const std::string &json) final;
  virtual messages::Transaction transaction(const messages::TransactionID &id) final;
  virtual const messages::UnspentTransactions list_unspent_transaction(const messages::Address &address) final;
  virtual bool
  set_inputs(messages::Transaction *transaction,
	     const messages::Address &sender,
	     const messages::NCCAmount &fees);
  virtual bool transaction_publish(const messages::Transaction &transaction) final;

  virtual const messages::Peers& peers() const final;
};


}  // api
}  // neuro

#endif /* NEURO_SRC_API_API_HPP */
