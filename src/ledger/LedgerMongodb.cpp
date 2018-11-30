#include "ledger/LedgerMongodb.hpp"
#include "common/logger.hpp"
#include "messages.pb.h"

namespace neuro {
namespace ledger {

mongocxx::instance LedgerMongodb::_instance{};

LedgerMongodb::LedgerMongodb(const std::string &url, const std::string &db_name)
    : _uri(url),
      _client(_uri),
      _db(_client[db_name]),
      _blocks(_db.collection("blocks")),
      _transactions(_db.collection("transactions")) {}

LedgerMongodb::LedgerMongodb(const messages::config::Database &config)
    : LedgerMongodb(config.url(), config.db_name()) {
  init_block0(config);
}

mongocxx::options::find LedgerMongodb::remove_OID() const {
  mongocxx::options::find find_options;
  auto projection_doc = bss::document{} << "_id" << 0
                                        << bss::finalize;  // remove _id
  find_options.projection(std::move(projection_doc));
  return find_options;
}

mongocxx::options::find LedgerMongodb::projection(
    const std::string &field) const {
  mongocxx::options::find find_options;
  auto projection_doc = bss::document{} << "_id" << 0 << field << 1
                                        << bss::finalize;
  find_options.projection(std::move(projection_doc));
  return find_options;
}

mongocxx::options::find LedgerMongodb::projection(
    const std::string &field0, const std::string &field1) const {
  mongocxx::options::find find_options;
  auto projection_transaction = bss::document{} << "_id" << 0 << field0 << 1
                                                << field1 << 1 << bss::finalize;
  find_options.projection(std::move(projection_transaction));
  return find_options;
}

bool LedgerMongodb::init_block0(const messages::config::Database &config) {
  messages::Block block0;
  if (get_block(0, &block0)) {
    return true;
  }
  block0.Clear();
  std::ifstream block0stream(config.block0_path());
  if (!block0stream.is_open()) {
    LOG_ERROR << "Could not load block from " << config.block0_path()
              << " from " << boost::filesystem::current_path().native();
    return false;
  }
  std::string str((std::istreambuf_iterator<char>(block0stream)),
                  std::istreambuf_iterator<char>());

  auto d = bss::document{};
  switch (config.block0_format()) {
    case messages::config::Database::Block0Format::_Database_Block0Format_PROTO:
      block0.ParseFromString(str);
      break;
    case messages::config::Database::Block0Format::_Database_Block0Format_BSON:
      d << str;
      messages::from_bson(d.view(), &block0);
      break;
    case messages::config::Database::Block0Format::_Database_Block0Format_JSON:
      messages::from_json(str, &block0);
      break;
  }

  messages::TaggedBlock tagged_block0;
  tagged_block0.set_branch(messages::Branch::MAIN);
  tagged_block0.add_branch_path(0);
  *tagged_block0.mutable_block() = block0;
  insert_block(&tagged_block0);

  //! add index to mongo collection
  _blocks.create_index(bss::document{} << "block.header.id" << 1
                                       << bss::finalize);
  _blocks.create_index(bss::document{} << "block.header.height" << 1
                                       << bss::finalize);
  _blocks.create_index(bss::document{} << "block.header.score" << 1
                                       << bss::finalize);
  _blocks.create_index(bss::document{} << "block.header.previousBlockHash" << 1
                                       << bss::finalize);
  _blocks.create_index(bss::document{} << "branch_path.0" << 1
                                       << bss::finalize);
  _transactions.create_index(bss::document{} << "transaction.id" << 1
                                             << bss::finalize);
  _transactions.create_index(bss::document{} << "blockId" << 1
                                             << bss::finalize);
  _transactions.create_index(bss::document{}
                             << "transaction.outputs.address.data" << 1
                             << bss::finalize);
  return true;
}

MongoQuery LedgerMongodb::query_branch(const messages::Branch &branch) const {
  return bss::document{} << "branch" << messages::Branch_Name(branch);
}

MongoQuery LedgerMongodb::query_main_branch() const {
  return query_branch(messages::Branch::MAIN);
}

void LedgerMongodb::remove_all() {
  std::lock_guard<std::mutex> lock(_ledger_mutex);
  _blocks.delete_many(bss::document{} << bss::finalize);
  _transactions.delete_many(bss::document{} << bss::finalize);
}

messages::BlockHeight LedgerMongodb::height() {
  std::lock_guard<std::mutex> lock(_ledger_mutex);
  auto query = query_main_branch() << bss::finalize;

  auto options = projection("block.header.height");
  options.sort(bss::document{} << "height" << -1 << bss::finalize);

  const auto result = _blocks.find_one(std::move(query), options);
  if (!result) {
    return 0;
  }

  return result->view()["block"]["header"]["height"].get_int32().value;
}

bool LedgerMongodb::is_ancestor(const messages::TaggedBlock &ancestor,
                                const messages::TaggedBlock &block) {
  if (ancestor.branch_path_size() == 0 ||
      ancestor.branch_path_size() > block.branch_path_size()) {
    return false;
  }
  for (int i = ancestor.branch_path_size() - 1; i >= 0; i--) {
    if (ancestor.branch_path(i) != block.branch_path(i)) {
      return false;
    }
  }
  return true;
}

bool LedgerMongodb::is_main_branch(
    const messages::TaggedTransaction &tagged_transaction) {
  messages::TaggedBlock tagged_block;
  return unsafe_get_block(tagged_transaction.block_id(), &tagged_block) &&
         tagged_block.branch() == messages::Branch::MAIN;
}

bool LedgerMongodb::get_block_header(const messages::BlockID &id,
                                     messages::BlockHeader *header) {
  std::lock_guard<std::mutex> lock(_ledger_mutex);
  auto query = bss::document{} << "block.header.id" << to_bson(id)
                               << bss::finalize;
  auto result = _blocks.find_one(std::move(query), projection("block.header"));
  if (!result) {
    return false;
  }
  messages::from_bson(result->view()["block"]["header"].get_document(), header);
  return true;
}

bool LedgerMongodb::get_last_block_header(messages::BlockHeader *block_header) {
  std::lock_guard<std::mutex> lock(_ledger_mutex);
  auto query = query_main_branch() << bss::finalize;
  auto options = projection("block.header");
  options.sort(bss::document{} << "height" << -1 << bss::finalize);

  const auto result = _blocks.find_one(std::move(query), options);
  if (!result) {
    return false;
  }

  messages::from_bson(result->view()["block"]["header"].get_document(),
                      block_header);
  return true;
}

bool LedgerMongodb::get_block_transactions(messages::Block *block) {
  assert(block->transactions().size() == 0);
  auto query = bss::document{} << "blockId" << to_bson(block->header().id())
                               << bss::finalize;
  auto options = projection("transaction");
  options.sort(bss::document{} << "transaction.id" << 1 << bss::finalize);
  auto cursor = _transactions.find(std::move(query), options);

  for (const auto &bson_transaction : cursor) {
    auto transaction = block->add_transactions();
    from_bson(bson_transaction["transaction"].get_document(), transaction);
  }

  return true;
}

bool LedgerMongodb::unsafe_get_block(const messages::BlockID &id,
                                     messages::TaggedBlock *tagged_block) {
  auto query = bss::document{} << "block.header.id" << to_bson(id)
                               << bss::finalize;
  auto result = _blocks.find_one(std::move(query), remove_OID());
  if (!result) {
    return false;
  }
  messages::from_bson(result->view(), tagged_block);
  get_block_transactions(tagged_block->mutable_block());
  return true;
}

bool LedgerMongodb::get_block(const messages::BlockID &id,
                              messages::TaggedBlock *tagged_block) {
  std::lock_guard<std::mutex> lock(_ledger_mutex);
  return unsafe_get_block(id, tagged_block);
}

bool LedgerMongodb::get_block(const messages::BlockID &id,
                              messages::Block *block) {
  std::lock_guard<std::mutex> lock(_ledger_mutex);
  auto query = bss::document{} << "block.header.id" << to_bson(id)
                               << bss::finalize;
  auto result = _blocks.find_one(std::move(query), projection("block"));
  if (!result) {
    return false;
  }
  messages::from_bson(result->view()["block"].get_document(), block);
  get_block_transactions(block);
  return true;
}

bool LedgerMongodb::get_block_by_previd(const messages::BlockID &previd,
                                        messages::Block *block) {
  // Look for a block in the main branch.
  // There may be several blocks with the same previd in forks.
  std::lock_guard<std::mutex> lock(_ledger_mutex);
  auto query = query_main_branch()
               << "previousBlockHash" << to_bson(previd) << bss::finalize;

  const auto result = _blocks.find_one(std::move(query), projection("block"));

  if (!result) {
    return false;
  }

  messages::from_bson(result->view()["block"].get_document(), block);
  get_block_transactions(block);
  return true;
}

bool LedgerMongodb::get_blocks_by_previd(
    const messages::BlockID &previd,
    std::vector<messages::TaggedBlock> *tagged_blocks) {
  std::lock_guard<std::mutex> lock(_ledger_mutex);
  auto query = bss::document{} << "previousBlockHash" << to_bson(previd)
                               << bss::finalize;

  auto bson_blocks = _blocks.find(std::move(query), remove_OID());

  if (bson_blocks.begin() == bson_blocks.end()) {
    return false;
  }

  for (const auto &bson_block : bson_blocks) {
    auto tagged_block = tagged_blocks->emplace_back();
    messages::from_bson(bson_block, &tagged_block);
    get_block_transactions(tagged_block.mutable_block());
  }

  return true;
}

bool LedgerMongodb::get_block(const messages::BlockHeight height,
                              messages::Block *block) {
  auto query = query_main_branch()
               << "block.header.height" << height << bss::finalize;
  const auto result = _blocks.find_one(std::move(query), projection("block"));
  if (!result) {
    return false;
  }

  messages::from_bson(result->view()["block"].get_document(), block);
  get_block_transactions(block);
  return true;
}

bool LedgerMongodb::insert_block(messages::TaggedBlock *tagged_block) {
  std::lock_guard<std::mutex> lock(_ledger_mutex);
  const auto header = tagged_block->block().header();
  auto bson_header = messages::to_bson(header);

  std::vector<bsoncxx::document::value> bson_transactions;
  for (const auto &transaction : tagged_block->block().transactions()) {
    messages::TaggedTransaction tagged_transaction;
    *tagged_transaction.mutable_transaction() = transaction;
    bson_transactions.push_back(messages::to_bson(tagged_transaction));
    if (tagged_block->branch() == messages::Branch::MAIN) {
      auto query = bss::document{}
                   << "transaction.id" << messages::to_bson(transaction.id())
                   << "blockId" << bsoncxx::types::b_null{} << bss::finalize;
      _transactions.delete_one(std::move(query));
    }
  }
  messages::TaggedBlock inserted_block;
  *inserted_block.mutable_block()->mutable_header() =
      tagged_block->block().header();
  inserted_block.set_branch(tagged_block->branch());
  tagged_block->mutable_block()->clear_transactions();
  auto bson_block = messages::to_bson(*tagged_block);
  _blocks.insert_one(std::move(bson_block));
  _transactions.insert_many(std::move(bson_transactions));
  return true;
}

bool LedgerMongodb::delete_block(const messages::Hash &id) {
  std::lock_guard<std::mutex> lock(_ledger_mutex);
  auto delete_block_query = bss::document{} << "block.header.id"
                                            << messages::to_bson(id)
                                            << bss::finalize;
  auto result = _blocks.delete_one(std::move(delete_block_query));
  if (result && result->deleted_count() > 0) {
    auto delete_transaction_query =
        bss::document{} << "blockId" << messages::to_bson(id) << bss::finalize;
    auto res_transaction =
        _transactions.delete_many(std::move(delete_transaction_query));
  }
  if (result) {
    return true;
  }
  return false;
}

bool LedgerMongodb::get_transaction(const messages::Hash &id,
                                    messages::Transaction *transaction) {
  messages::BlockHeight block_height;
  return get_transaction(id, transaction, &block_height);
}

bool LedgerMongodb::get_transaction(const messages::Hash &id,
                                    messages::Transaction *transaction,
                                    messages::BlockHeight *blockheight) {
  std::lock_guard<std::mutex> lock(_ledger_mutex);
  auto query_transaction = bss::document{} << "transaction.id"
                                           << messages::to_bson(id)
                                           << bss::finalize;

  auto bson_transactions =
      _transactions.find(std::move(query_transaction), remove_OID());

  for (const auto &bson_transaction : bson_transactions) {
    messages::TaggedTransaction tagged_transaction;
    messages::from_bson(bson_transaction, &tagged_transaction);
    messages::TaggedBlock tagged_block;
    if (unsafe_get_block(tagged_transaction.block_id(), &tagged_block) &&
        tagged_block.branch() == messages::Branch::MAIN) {
      *transaction = tagged_transaction.transaction();
      *blockheight = tagged_block.block().header().height();
      return true;
    }
  }
  return false;
}

int LedgerMongodb::total_nb_transactions() {
  std::lock_guard<std::mutex> lock(_ledger_mutex);
  return 0;  // TODO this may be slow...
}

int LedgerMongodb::total_nb_blocks() {
  std::lock_guard<std::mutex> lock(_ledger_mutex);
  auto query = query_main_branch() << bss::finalize;
  return _blocks.count(std::move(query));
}

bool LedgerMongodb::for_each(const Filter &filter, Functor functor) {
  std::lock_guard<std::mutex> lock(_ledger_mutex);
  if (!filter.output_address() && !filter.input_transaction_id()) {
    LOG_WARNING << "missing filters for for_each query";
    return false;
  }

  auto query = bss::document{};

  if (filter.output_address()) {
    const auto bson = messages::to_bson(*filter.output_address());
    query << "transaction.outputs.address" << bson;
  }

  if (filter.input_transaction_id()) {
    query << "transaction.inputs.id"
          << messages::to_bson(*filter.input_transaction_id());
  }

  if (filter.output_id()) {
    query << "transaction.inputs.outputId" << *filter.output_id();
  }

  LOG_DEBUG << "for_each query transaction " << bsoncxx::to_json(query);
  auto bson_transactions =
      _blocks.find(std::move(query << bss::finalize), remove_OID());

  bool applied_functor = false;
  for (const auto &bson_transaction : bson_transactions) {
    messages::TaggedTransaction tagged_transaction;
    from_bson(bson_transaction, &tagged_transaction);
    if (is_main_branch(tagged_transaction)) {
      functor(tagged_transaction.transaction());
      applied_functor = true;
    }
  }

  return applied_functor;
}

int LedgerMongodb::new_branch_id() {
  auto query = bss::document{} << bss::finalize;
  mongocxx::options::find find_options;
  auto projection_doc = bss::document{} << "_id" << 0 << "branch_path"
                                        << bss::document{} << "$slice" << 1
                                        << bss::finalize;
  find_options.projection(std::move(projection_doc));
  find_options.sort(bss::document{} << "branch_path.0" << -1 << bss::finalize);

  auto max_branch_id = _blocks.find_one(std::move(query), find_options);
  return max_branch_id->view()["branch_path"][0].get_int32() + 1;
}

}  // namespace ledger
}  // namespace neuro
