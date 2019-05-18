#ifndef NEURO_SRC_API_API_HPP
#define NEURO_SRC_API_API_HPP

#include "messages/Message.hpp"
#include "messages/Address.hpp"

namespace neuro {
class Bot;
namespace api {

class Api {
 private:
  Bot *_bot;
  
 public:
  Api(Bot *bot);
  virtual ~Api(){}

 protected:
  
  virtual bool verify_address_format(const messages::Address &address) final;
  virtual messages::NCCAmount balance(const messages::Address &address) final;
  virtual messages::Addresses previous_recipients(const messages::Address &address) final;

  virtual messages::BlockHeight height() const final;
  virtual bool is_ledger_uptodate() const final;

  virtual Buffer transaction(const messages::Transaction &transaction) const final;
  virtual std::optional<messages::Transaction> transaction_from_json(const std::string &json) final;
  virtual messages::Transaction transaction(const messages::TransactionID &id) final;
  virtual bool
  set_inputs(messages::Transaction *transaction,
	     const messages::Address &sender,
	     const messages::NCCAmount &fees);
  // virtual messages::Transactions transactions_in(const messages::Address &address) final;
  // virtual messages::Transactions transactions_out(const messages::Address &address) final;
  // virtual messages::Transactions transactions_unspent(const messages::Address &address) final;
  // virtual messages::Transactions transaction_pool() final;
  // virtual bool transaction_publish(const Buffer &buffer) final;
};


}  // api
}  // neuro

#endif /* NEURO_SRC_API_API_HPP */
