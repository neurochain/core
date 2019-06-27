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

messages::Block Api::block(messages::BlockID id) {
  messages::Block block;
  _bot->ledger()->get_block(id, &block);
  return block;
}

messages::Blocks Api::last_blocks(std::size_t nb_blocks) {
  messages::Blocks blocks;
  auto last_blocks = _bot->ledger()->get_last_blocks(nb_blocks);
  for (auto& last_block : last_blocks) {
    blocks.add_block()->CopyFrom(last_block);
  }
  return blocks;
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

const messages::UnspentTransactions
Api::list_unspent_transaction(const messages::Address &address) {
  const auto &unspent_transactions =
      _bot->ledger()->list_unspent_transactions(address);
  messages::UnspentTransactions unspent_transactions_message;
  for (auto &unspent_transaction : unspent_transactions) {
    unspent_transactions_message.add_unspent_transactions()->CopyFrom(
        unspent_transaction);
  }
  return unspent_transactions_message;
}

bool
Api::set_inputs(messages::Transaction *transaction,
		const messages::Address &sender,
		const messages::NCCAmount &fees) {
  
  return _bot->ledger()->set_inputs(transaction, sender, fees);
  
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
