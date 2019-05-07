#ifndef NEURO_SRC_API_API_HPP
#define NEURO_SRC_API_API_HPP

#include "messages/Message.hpp"

namespace neuro {
class Bot;
namespace api {

class Api {
 private:
  Bot *_bot;
  
 public:
  Api(Bot *bot) : _bot(bot) {}

 protected:
  
  virtual bool verify_address_format(const messages::Address &address) final;
  virtual messages::NCCAmount balance(const messages::Address &address) final;
  virtual messages::Addresses previous_recipients(const messages::Address &address) final;

  virtual messages::BlockHeight height() const final;
  virtual bool is_ledger_uptodate() const final;

  virtual Buffer transaction(const messages::Transaction &transaction) final;
  virtual std::optional<messages::Transaction> transaction_from_json(const std::string &json) final;
  virtual messages::Transaction transaction(const messages::TransactionID &id) final;
  virtual messages::Transaction transaction_prefilled(const messages::Addresses &address_in) final;
  virtual messages::Transactions transactions_in(const messages::Address &address) final;
  virtual messages::Transactions transactions_out(const messages::Address &address) final;
  virtual messages::Transactions transactions_unspent(const messages::Address &address) final;
  virtual messages::Transactions transaction_pool() final;
  virtual bool transaction_publish(const Buffer &buffer) final;
  
  virtual ~Api(){}
};


}  // api
}  // neuro

#endif /* NEURO_SRC_API_API_HPP */
