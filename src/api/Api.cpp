#include "api/Api.hpp"
#include "Bot.hpp"

namespace neuro {

namespace api {

Api::Api(Bot *bot) : _bot(bot) {
  messages::Block block;
  _bot->ledger()->get_block(int32_t{0}, &block, true);
}

bool Api::verify_key_pub_format(const messages::_KeyPub &key_pub) {
  // TODO
  return false;
}

messages::NCCAmount Api::balance(const messages::_KeyPub &key_pub) {
  return _bot->ledger()->balance(key_pub);
}

std::vector<messages::_KeyPub> Api::previous_recipients(
    const messages::_KeyPub &key_pub) {
  return {};
}

messages::BlockHeight Api::height() const { return _bot->ledger()->height(); }

messages::Block Api::block(messages::BlockID id) {
  messages::Block block;
  _bot->ledger()->get_block(id, &block);
  return block;
}

messages::Block Api::block(messages::BlockHeight height) {
  messages::Block block;
  _bot->ledger()->get_block(height, &block);
  return block;
}

messages::Blocks Api::last_blocks(std::size_t nb_blocks) {
  messages::Blocks blocks;
  auto last_blocks = _bot->ledger()->get_last_blocks(nb_blocks);
  for (const auto &last_block : last_blocks) {
    blocks.add_block()->CopyFrom(last_block);
  }
  return blocks;
}

std::size_t Api::total_nb_transactions() const {
  return _bot->ledger()->total_nb_transactions();
}
std::size_t Api::total_nb_blocks() const {
  return _bot->ledger()->total_nb_blocks();
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

messages::Transaction Api::build_transaction(
    const messages::_KeyPub &sender_key_pub,
    const google::protobuf::RepeatedPtrField<messages::Output> &outputs,
    const std::optional<messages::NCCAmount> &fees) {
  auto transaction = _bot->ledger()->build_transaction(
      sender_key_pub, outputs, fees);
  transaction.mutable_id()->set_type(messages::Hash::SHA256);
  transaction.mutable_id()->set_data("");
  return transaction;
}

std::optional<messages::Transaction> Api::transaction_from_json(
    const std::string &json) {
  messages::Transaction transaction;
  return (messages::from_json(json, &transaction))
             ? std::make_optional(transaction)
             : std::nullopt;
}

messages::Transaction Api::transaction(const messages::TransactionID &id) {
  messages::Transaction transaction;
  _bot->ledger()->get_transaction(id, &transaction);
  return transaction;
}

const messages::Peers &Api::peers() const { return _bot->peers(); }

// messages::Transactions Api::transactions_in(const messages::_KeyPub &key_pub)
// {

// }

// messages::Transactions Api::transactions_out(const messages::_KeyPub
// &key_pub){

// }

// messages::Transactions Api::transactions_unspent(const messages::_KeyPub
// &key_pub){

// }

// messages::Transactions Api::transaction_pool(){

// }

bool Api::transaction_publish(const messages::Transaction &transaction) {
  return _bot->publish_transaction(transaction);
}

}  // namespace api

}  // namespace neuro
