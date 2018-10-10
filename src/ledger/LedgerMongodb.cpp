#include "ledger/LedgerMongodb.hpp"
#include "messages.pb.h"

namespace neuro {
namespace ledger {

LedgerMongodb::LedgerMongodb(const std::string &url, const std::string &db_name)
    : _uri(url),
      _client(_uri),
      _db(_client[db_name]),
      _blocks(_db.collection("blocks")),
      _transactions(_db.collection("transactions")) {}

LedgerMongodb::LedgerMongodb(messages::config::Database &db)
    : _uri((std::string)db.url()),
      _client(_uri),
      _db(_client[(std::string)db.db_name()]),
      _blocks(_db.collection("blocks")),
      _transactions(_db.collection("transactions")) {
  init_block0(db);
}

//   messages::BlockHeight LedgerMongodb::height() const {
//   auto query = bss::document{} << bss::finalize;
//   auto options = mongocxx::options::find{};
//   options.sort(bss::document{} << "height" << -1 << bss::finalize);
//   const auto res = _blocks.find_one(std::move(query), options);
//   if (!res) {
//     return 0;
//   }

//   return res->view()["height"].get_int32().value + 1;
// }

// messages::Transaction::State LedgerMongodb::get_block(const messages::BlockID
// &id,
//                                                       messages::Block *block)
//                                                       {
//   // const auto state = get_block_header(id, block->mutable_header());
//   // const auto bson_id =
//   //     bsoncxx::types::b_binary{bsoncxx::binary_sub_type::k_binary,
//   //                              static_cast<uint32_t>(id.size()),
//   id.data()};
//   // auto query = bss::document{} << "_id" << bson_id << bss::finalize;
//   // auto cursor = _transactions.find(std::move(query));
//   // get_block(cursor, block);
//   // return state;
// }

// messages::Transaction::State LedgerMongodb::get_block(
//     const messages::BlockHeight height, messages::Block *block) {
//   return messages::Transaction::UNKNOWN; // STUB
// }

// messages::Transaction::State LedgerMongodb::get_last_block(messages::Block
// *block) {
//   return messages::Transaction::UNKNOWN;  // STUB
// }

// bool LedgerMongodb::push_block(const messages::Block &block) {
//   return false;
// }  // STUB
// bool LedgerMongodb::delete_block(const messages::BlockID &id) {
//   return false;
// }  // STUB
// bool LedgerMongodb::for_each(Filter filter, Functor functor) {
//   return false;
// }  // STUB
// messages::Transaction::State LedgerMongodb::get_transaction(
//     const messages::TransactionID &id, messages::Transaction *transaction) {
//   return messages::Transaction::UNKNOWN;
// }  // STUB
// bool LedgerMongodb::has_transaction(
//     const messages::TransactionID &transaction_id) const {
//   return false;
// }  // STUB
// bool LedgerMongodb::has_transaction(
//     const messages::BlockID &block_id,
//     const messages::TransactionID &transaction_id) const {
//   return false;
// }  // STUB
// messages::Transaction::State LedgerMongodb::transaction(
//     const messages::BlockID &block_id, const messages::TransactionID &id,
//     messages::Transaction *transaction) const {
//   return messages::Transaction::UNKNOWN;
// }  // STUB

}  // namespace ledger
}  // namespace neuro
