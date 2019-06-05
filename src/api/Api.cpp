#include "api/Api.hpp"
#include "Bot.hpp"

namespace neuro {

namespace api {

Api::Api(Bot *bot) : _bot(bot) {
    messages::Block block;
    _bot->ledger()->get_block(int32_t{0}, &block, true);
    std::cout << messages::to_json(block) << std::endl;
}
  

bool Api::verify_address_format(const messages::Address &address) {
  // TODO
  return false;
}
  
messages::NCCAmount Api::balance(const messages::Address &address) {
  return _bot->ledger()->balance(address);
}

messages::Addresses Api::previous_recipients(const messages::Address &address) {
  return {};
}

messages::BlockHeight Api::height() const {
  return _bot->ledger()->height();
}

bool Api::is_ledger_uptodate() const {
  // TODO 
  return false;
}


Buffer Api::transaction(const messages::Transaction &transaction) const {
  Buffer buffer;
  messages::to_buffer(transaction, &buffer);
  return buffer;
}

std::optional<messages::Transaction> Api::transaction_from_json(const std::string &json){
  messages::Transaction transaction;
  return (messages::from_json(json, &transaction)) ? std::make_optional(transaction): std::nullopt;
}

messages::Transaction Api::transaction(const messages::TransactionID &id){
  messages::Transaction transaction;
  _bot->ledger()->get_transaction(id, &transaction);
  return transaction;
}

bool
Api::set_inputs(messages::Transaction *transaction,
		const messages::Address &sender,
		const messages::NCCAmount &fees) {
  
  return false;// _bot->ledger()->set_inputs(transaction, sender, fees);
  
}

// messages::Transactions Api::transactions_in(const messages::Address &address) {

// }

// messages::Transactions Api::transactions_out(const messages::Address &address){

// }

// messages::Transactions Api::transactions_unspent(const messages::Address &address){

// }

// messages::Transactions Api::transaction_pool(){

// }
  
bool Api::transaction_publish(const messages::Transaction &transaction) {
  return _bot->publish_transaction(transaction);
}



  

}  // api


}  // neuro
