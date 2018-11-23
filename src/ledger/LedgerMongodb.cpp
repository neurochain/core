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
      _blocks(_db.collection("blocks")) {}

LedgerMongodb::LedgerMongodb(const messages::config::Database &config)
    : LedgerMongodb(config.url(), config.db_name()) {
  init_block0(config);
}

mongocxx::options::find LedgerMongodb::remove_OID() const {
  mongocxx::options::find find_options;
  auto projection_transaction = bss::document{} << "_id" << 0
                                                << bss::finalize;  // remove _id
  find_options.projection(std::move(projection_transaction));
  return find_options;
}

mongocxx::options::find LedgerMongodb::projection(
    const std::string &field) const {
  mongocxx::options::find find_options;
  auto projection_transaction = bss::document{} << "_id" << 0 << field << 1
                                                << bss::finalize;
  find_options.projection(std::move(projection_transaction));
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
  messages::Block block0file;
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
      block0file.ParseFromString(str);
      break;
    case messages::config::Database::Block0Format::_Database_Block0Format_BSON:
      d << str;
      messages::from_bson(d.view(), &block0file);
      break;
    case messages::config::Database::Block0Format::_Database_Block0Format_JSON:
      messages::from_json(str, &block0file);
      break;
  }
  push_block(block0file);

  //! add index to mongo collection
  _blocks.create_index(bss::document{} << "block.header.id" << 1
                                       << bss::finalize);
  _blocks.create_index(bss::document{} << "block.header.height" << 1
                                       << bss::finalize);
  _blocks.create_index(bss::document{} << "block.header.score" << 1
                                       << bss::finalize);
  _blocks.create_index(bss::document{} << "block.header.previousBlockHash" << 1
                                       << bss::finalize);
  _blocks.create_index(bss::document{} << "block.transactions.id" << 1
                                       << bss::finalize);
  _blocks.create_index(bss::document{}
                       << "block.transactions.outputs.address.data" << 1
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

  messages::from_bson(result->view()["block.header"].get_document(),
                      block_header);
  return true;
}

bool LedgerMongodb::get_block(const messages::BlockID &id,
                              messages::TaggedBlock *tagged_block) {
  std::lock_guard<std::mutex> lock(_ledger_mutex);
  auto query = bss::document{} << "block.header.id" << to_bson(id)
                               << bss::finalize;
  auto result = _blocks.find_one(std::move(query), remove_OID());
  if (!result) {
    return false;
  }
  messages::from_bson(result->view(), tagged_block);
  return true;
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

  for (auto &&bson_block : bson_blocks) {
    auto tagged_block = tagged_blocks->emplace_back();
    messages::from_bson(bson_block, &tagged_block);
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
  return true;
}

bool LedgerMongodb::insert_block(const messages::Block &block,
                                 const messages::Branch &branch) {
  std::lock_guard<std::mutex> lock(_ledger_mutex);
  const auto header = block.header();
  auto bson_header = messages::to_bson(header);

  messages::TaggedBlock tagged_block;
  *tagged_block.mutable_block() = block;
  tagged_block.set_branch(messages::Branch::MAIN);
  auto bson_block = messages::to_bson(tagged_block);
  _blocks.insert_one(std::move(bson_block));
  return true;
}

bool LedgerMongodb::push_block(const messages::Block &block) {
  // TODO remove transactions from the transaction pool?
  return insert_block(block, messages::Branch::MAIN);
}

bool LedgerMongodb::delete_block(const messages::Hash &id) {
  std::lock_guard<std::mutex> lock(_ledger_mutex);
  auto delete_block_query = bss::document{} << "block.header.id"
                                            << messages::to_bson(id)
                                            << bss::finalize;
  auto result = _blocks.delete_one(std::move(delete_block_query));
  if (result) {
    return true;
  }
  return false;
}

bool LedgerMongodb::get_transaction(const messages::Hash &id,
                                    messages::Transaction *transaction) {
  std::lock_guard<std::mutex> lock(_ledger_mutex);
  auto query_transaction = bss::document{} << "block.transactions.id"
                                           << messages::to_bson(id)
                                           << bss::finalize;

  auto result = _blocks.find_one(std::move(query_transaction),
                                 projection("block.transactions.$"));
  if (result) {
    messages::from_bson(
        result->view()["block"]["transactions"][0].get_document(), transaction);
    return true;
  }
  return false;
}

bool LedgerMongodb::get_transaction(const messages::Hash &id,
                                    messages::Transaction *transaction,
                                    messages::BlockHeight *blockheight) {
  std::lock_guard<std::mutex> lock(_ledger_mutex);
  auto query_transaction = bss::document{} << "block.transactions.id"
                                           << messages::to_bson(id)
                                           << bss::finalize;

  auto result = _blocks.find_one(
      std::move(query_transaction),
      projection("block.transactions.$", "block.header.height"));
  if (result) {
    messages::from_bson(
        result->view()["block"]["transactions"][0].get_document(), transaction);
    messages::BlockHeader header;

    // Ideally we would get only the height but then parsing it is dependent on
    // its type
    messages::from_bson(result->view()["block"]["header"].get_document(),
                        &header);
    *blockheight = header.height();
    return true;
  }
  return false;
}

int LedgerMongodb::total_nb_transactions() {
  std::lock_guard<std::mutex> lock(_ledger_mutex);
  return 0;  // TODO
}

int LedgerMongodb::total_nb_blocks() {
  std::lock_guard<std::mutex> lock(_ledger_mutex);
  auto query = query_main_branch() << bss::finalize;
  return _blocks.count(std::move(query));
}

bool LedgerMongodb::for_each(const Filter &filter, Functor functor) {
  std::lock_guard<std::mutex> lock(_ledger_mutex);
  // TODO
  return false;
}

}  // namespace ledger
}  // namespace neuro
