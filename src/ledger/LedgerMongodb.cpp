#include "ledger/LedgerMongodb.hpp"
#include "messages.pb.h"

namespace neuro {
namespace ledger {

mongocxx::instance LedgerMongodb::_instance{};

LedgerMongodb::LedgerMongodb(const std::string &url, const std::string &db_name)
    : _uri(url),
      _client(_uri),
      _db(_client[db_name]),
      _blocks(_db.collection("blocks")),
      _transactions(_db.collection("transactions")),
      _blocks_forks(_db.collection("blocksfork")) {}

LedgerMongodb::LedgerMongodb(const messages::config::Database &db)
    : LedgerMongodb(db.url(), db.db_name()) {
  init_block0(db);
}

mongocxx::options::find LedgerMongodb::remove_OID() {
  mongocxx::options::find findoption;
  auto projection_transaction =
      bss::document{} << "_id" << 0 << bss::finalize;  ///!< remove _id objectID
  findoption.projection(std::move(projection_transaction));
  return findoption;
}

bool LedgerMongodb::get_block_header(const bsoncxx::document::view &block,
                                     messages::BlockHeader *header) {
  messages::from_bson(block, header);
  return true;
}

bool LedgerMongodb::get_transactions_from_block(
    const bsoncxx::document::view &id, messages::Block *block) {
  auto query = bss::document{} << "blockId" << id << bss::finalize;
  auto cursor = _transactions.find(std::move(query), remove_OID());

  for (auto &bson_transaction : cursor) {
    auto transaction = block->add_transactions();
    from_bson(bson_transaction, transaction);
  }

  return true;
}

bool LedgerMongodb::get_transactions_from_block(const messages::BlockID &id,
                                                messages::Block *block) {
  const auto bson_id = to_bson(id);
  return get_transactions_from_block(bson_id.view(), block);
}

void LedgerMongodb::init_block0(const messages::config::Database &db) {
  messages::Block block0;
  if (get_block(0, &block0)) {
    return;
  }
  messages::Block block0file;
  std::ifstream t(db.block0_path());
  std::string str((std::istreambuf_iterator<char>(t)),
                  std::istreambuf_iterator<char>());

  auto d = bss::document{};
  switch (db.block0_format()) {
    case messages::config::Database::Block0Format::Database_Block0Format_PROTO:
      block0file.ParseFromString(str);
      break;
    case messages::config::Database::Block0Format::Database_Block0Format_BSON:
      d << str;
      messages::from_bson(d.view(), &block0file);
      break;
    case messages::config::Database::Block0Format::Database_Block0Format_JSON:
      messages::from_json(str, &block0file);
      break;
  }
  push_block(block0file);

  //! add index to mongo collection
  _blocks.create_index(bss::document{} << "id" << 1 << bss::finalize);
  _blocks.create_index(bss::document{} << "height" << 1 << bss::finalize);
  _blocks.create_index(bss::document{} << "previousBlockHash" << 1
                                       << bss::finalize);
  _transactions.create_index(bss::document{} << "id" << 1 << bss::finalize);
  _transactions.create_index(bss::document{} << "blockId" << 1
                                             << bss::finalize);
  _transactions.create_index(bss::document{} << "outputs.address.data" << 1
                                             << bss::finalize);

  _blocks_forks.create_index(bss::document{} << "header.id" << 1
                                             << bss::finalize);
  _blocks_forks.create_index(bss::document{} << "header.height" << 1
                                             << bss::finalize);
}

void LedgerMongodb::remove_all() {
  _blocks.delete_many(bss::document{} << bss::finalize);
  _blocks_forks.delete_many(bss::document{}.view());
  _transactions.delete_many(bss::document{}.view());
}

messages::BlockHeight LedgerMongodb::height() {
  std::lock_guard<std::mutex> lock(_ledger_mutex);
  auto query = bss::document{} << bss::finalize;

  mongocxx::options::find options;
  auto projection_transaction =
      bss::document{} << "_id" << 0 << bss::finalize;  ///!< remove _id objectID
  options.projection(projection_transaction.view());
  options.sort(bss::document{} << "height" << -1 << bss::finalize);

  const auto res = _blocks.find_one(std::move(query), options);
  if (!res) {
    return 0;
  }

  return res->view()["height"].get_int32().value;
}

bool LedgerMongodb::get_block_header(const messages::BlockID &id,
                                     messages::BlockHeader *header) {
  std::lock_guard<std::mutex> lock(_ledger_mutex);
  const auto bson_id = to_bson(id);
  auto query = bss::document{} << "id" << bson_id << bss::finalize;
  auto res = _blocks.find_one(std::move(query), remove_OID());
  if (!res) {
    return false;
  }
  return get_block_header(res->view(), header);
}

bool LedgerMongodb::get_last_block_header(messages::BlockHeader *block_header) {
  std::lock_guard<std::mutex> lock(_ledger_mutex);
  auto query = bss::document{} << bss::finalize;
  auto options = remove_OID();
  options.sort(bss::document{} << "height" << -1 << bss::finalize);

  const auto res = _blocks.find_one(std::move(query), options);
  if (!res) {
    return false;
  }

  messages::from_bson(*res, block_header);
  return true;
}

bool LedgerMongodb::get_block(const messages::BlockID &id,
                              messages::Block *block) {
  // std::lock_guard<std::mutex> lock(_ledger_mutex);
  auto header = block->mutable_header();
  auto res_state = get_block_header(id, header);

  res_state &= get_transactions_from_block(id, block);

  return res_state;
}

bool LedgerMongodb::get_block_by_previd(const messages::BlockID &previd,
                                        messages::Block *block) {
  std::lock_guard<std::mutex> lock(_ledger_mutex);
  auto previd_query = bss::document{} << "previousBlockHash"
                                      << messages::to_bson(previd)
                                      << bss::finalize;

  const auto res = _blocks.find_one(std::move(previd_query), remove_OID());

  if (!res) {
    return false;
  }

  auto header = block->mutable_header();
  get_block_header(res->view(), header);

  get_transactions_from_block(res->view()["id"].get_document(), block);

  return true;
}

bool LedgerMongodb::get_block(const messages::BlockHeight height,
                              messages::Block *block) {
  std::lock_guard<std::mutex> lock(_ledger_mutex);
  auto query = bss::document{} << "height" << height << bss::finalize;
  const auto res = _blocks.find_one(std::move(query), remove_OID());
  if (!res) {
    return false;
  }

  auto header = block->mutable_header();
  get_block_header(res->view(), header);

  get_transactions_from_block(res->view()["id"].get_document(), block);

  return true;
}

bool LedgerMongodb::push_block(const messages::Block &block) {
  std::lock_guard<std::mutex> lock(_ledger_mutex);
  const auto header = block.header();
  auto bson_header = messages::to_bson(header);

  _blocks.insert_one(std::move(bson_header));

  for (neuro::messages::Transaction transaction : block.transactions()) {
    if (!transaction.has_id()) {
      neuro::messages::Transaction _transaction(transaction);
      _transaction.clear_id();
      _transaction.clear_block_id();

      Buffer buf;
      messages::to_buffer(_transaction, &buf);
      messages::Hasher newit(buf);
      transaction.mutable_id()->CopyFrom(newit);
    }

    if (!transaction.has_block_id()) {
      transaction.mutable_block_id()->CopyFrom(header.id());
    }

    auto bson_transaction = messages::to_bson(transaction);
    _transactions.insert_one(std::move(bson_transaction));
  }
  return true;
}

bool LedgerMongodb::delete_block(const messages::Hash &id) {
  std::lock_guard<std::mutex> lock(_ledger_mutex);
  auto delete_block_query = bss::document{} << "id" << messages::to_bson(id)
                                            << bss::finalize;
  auto res = _blocks.delete_one(std::move(delete_block_query));
  if (res) {
    auto delete_transaction_query =
        bss::document{} << "blockId" << messages::to_bson(id) << bss::finalize;
    auto res_transaction =
        _transactions.delete_many(delete_transaction_query.view());
    if (res_transaction) {
      return true;
    }
  }
  return false;
}

bool LedgerMongodb::get_transaction(const messages::Hash &id,
                                    messages::Transaction *transaction) {
  std::lock_guard<std::mutex> lock(_ledger_mutex);
  auto query_transaction = bss::document{} << "id" << messages::to_bson(id)
                                           << bss::finalize;

  auto res = _transactions.find_one(std::move(query_transaction), remove_OID());
  if (res) {
    messages::from_bson(res->view(), transaction);
    return true;
  }
  return false;
}

bool LedgerMongodb::add_transaction(const messages::Transaction &transaction) {
  std::lock_guard<std::mutex> lock(_ledger_mutex);
  auto bson_transaction = messages::to_bson(transaction);
  auto res = _transactions.insert_one(std::move(bson_transaction));
  if (res) {
    return true;
  }
  return false;
}

bool LedgerMongodb::delete_transaction(const messages::Hash &id) {
  std::lock_guard<std::mutex> lock(_ledger_mutex);
  auto query_transaction = bss::document{} << "id" << messages::to_bson(id)
                                           << bss::finalize;
  auto res = _transactions.delete_one(std::move(query_transaction));
  if (res) {
    return true;
  }
  return false;
}

int LedgerMongodb::total_nb_transactions() {
  std::lock_guard<std::mutex> lock(_ledger_mutex);
  auto query = bss::document{} << "blockId" << bss::open_document << "$exists"
                               << 1 << bss::close_document << bss::finalize;
  return _transactions.count(std::move(query));
}

int LedgerMongodb::total_nb_blocks() {
  std::lock_guard<std::mutex> lock(_ledger_mutex);
  return _blocks.count({});
}

int LedgerMongodb::get_transaction_pool(messages::Block &block) {
  std::lock_guard<std::mutex> lock(_ledger_mutex);
  auto query_transaction_pool = bss::document{}
                                << "blockId" << bss::open_document << "$exists"
                                << 0 << bss::close_document << bss::finalize;

  // TO DO add filter for order transaction here #Trx1

  int transactions_num = 0;
  auto cursor =
      _transactions.find(std::move(query_transaction_pool), remove_OID());
  for (auto &bson_transaction : cursor) {
    auto *transaction = block.add_transactions();
    messages::from_bson(bson_transaction, transaction);
    transactions_num++;
  }
  return transactions_num;
}

bool LedgerMongodb::for_each(const Filter &filter, Functor functor) {
  std::lock_guard<std::mutex> lock(_ledger_mutex);
  if (!filter.output() && !filter.input_transaction_id()) {
    return false;
  }

  bss::document query_block;

  ///!< TO DO
  /*
  if (filter.lower_height())
  {
      query_block << "height"
                  << bss::open_document
                  << "$gte" << *filter.lower_height()
                  << bss::close_document;
  }
  if (filter.upper_height())
  {
      query_block << "height"
                  << bss::open_document
                  << "$lte" << *filter.upper_height()
                  << bss::close_document;
  }

  if (filter.block_id())
  {
      const auto bson = messages::to_bson(*filter.block_id());
      query_block << "id" << bson;
  }
  */

  query_block << bss::finalize;

  auto query_transaction = bss::document{};

  if (filter.output()) {
    const auto bson = messages::to_bson(*filter.output());
    query_transaction << "outputs.address" << bson;
  }

  if (filter.input_transaction_id()) {
    query_transaction << "inputs.id"
                      << messages::to_bson(*filter.input_transaction_id());
  }

  if (filter.output_id()) {
    query_transaction << "inputs.outputId" << *filter.output_id();
  }

  // auto cursor_block = _blocks.find(query_block.view());
  auto query_final = query_transaction << bss::finalize;
  auto cursor_transaction =
      _transactions.find(std::move(query_final), remove_OID());

  for (auto &&bson_transaction : cursor_transaction) {
    messages::Transaction transaction;
    messages::from_bson(bson_transaction, &transaction);
    functor(transaction);
  }

  return true;
}

bool LedgerMongodb::get_blocks(int start, int size,
                               std::vector<messages::Block> &blocks) {
  std::lock_guard<std::mutex> lock(_ledger_mutex);
  blocks.clear();
  auto query_block = bss::document{} << bss::finalize;
  mongocxx::options::find filter = remove_OID();
  filter.sort(bss::document{} << "height" << 1 << bss::finalize);
  filter.skip(start).limit(size);

  auto cursor_transaction = _blocks.find(std::move(query_block), filter);

  if (cursor_transaction.begin() == cursor_transaction.end()) {
    return false;
  }

  for (auto &&bsonheader : cursor_transaction) {
    messages::Block block;
    messages::from_bson(bsonheader, block.mutable_header());
    get_transactions_from_block(block.header().id(), &block);
    blocks.push_back(block);
  }

  return true;
}

bool LedgerMongodb::fork_add_block(const messages::Block &block) {
  std::lock_guard<std::mutex> lock(_ledger_mutex);
  messages::Block block_updated(block);

  for (int i = 0; i < block_updated.transactions_size(); ++i) {
    neuro::messages::Transaction *transaction =
        block_updated.mutable_transactions(i);
    if (!transaction->has_id()) {
      neuro::messages::Transaction _transaction(*transaction);
      _transaction.clear_id();
      _transaction.clear_block_id();

      Buffer buf;
      messages::to_buffer(_transaction, &buf);
      messages::Hasher newit(buf);
      transaction->mutable_id()->CopyFrom(newit);
    }

    if (!transaction->has_block_id()) {
      transaction->mutable_block_id()->CopyFrom(block.header().id());
    }
  }

  auto bson_block = messages::to_bson(block_updated);
  auto res = _blocks_forks.insert_one(std::move(bson_block));
  if (res) {
    return true;  //(res->result().inserted_count () == 1);
  }
  return false;
}

bool LedgerMongodb::fork_delete_block(const messages::Hash &id) {
  std::lock_guard<std::mutex> lock(_ledger_mutex);
  auto query_block = bss::document{} << "header.id" << messages::to_bson(id)
                                     << bss::finalize;
  auto res = _blocks_forks.delete_one(std::move(query_block));
  if (res) {
    return true;
  }
  return false;
}

void LedgerMongodb::fork_for_each(Functor_block functor) {
  std::lock_guard<std::mutex> lock(_ledger_mutex);
  auto query_block = bss::document{} << bss::finalize;

  auto findoption = remove_OID();
  findoption.sort(bss::document{} << "header.height" << 1 << bss::finalize);

  auto cursor_block = _blocks_forks.find(std::move(query_block), findoption);

  for (auto &bson_block : cursor_block) {
    messages::Block block;
    messages::from_bson(bson_block, &block);
    functor(block);
  }
}

bool LedgerMongodb::fork_get_block(const messages::BlockID &id,
                                   messages::Block *block) {
  std::lock_guard<std::mutex> lock(_ledger_mutex);
  auto query = bss::document{} << "header.id" << messages::to_bson(id)
                               << bss::finalize;

  auto findoption = remove_OID();
  const auto res = _blocks_forks.find_one(std::move(query), findoption);
  if (!res) {
    return false;
  }

  messages::from_bson(res->view(), block);
  return true;
}

bool LedgerMongodb::fork_get_block(const messages::BlockHeight height,
                                   messages::Block *block) {
  std::lock_guard<std::mutex> lock(_ledger_mutex);
  auto query = bss::document{} << "header.height" << height << bss::finalize;

  const auto res = _blocks_forks.find_one(std::move(query), remove_OID());
  if (!res) {
    return false;
  }

  messages::from_bson(res->view(), block);
  return true;
}

void LedgerMongodb::fork_test() {
  std::lock_guard<std::mutex> lock(_ledger_mutex);
  auto query_block = bss::document{} << bss::finalize;
  _blocks_forks.delete_many(std::move(query_block));
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
