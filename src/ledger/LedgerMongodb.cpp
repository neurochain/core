#include "ledger/LedgerMongodb.hpp"
#include "common/logger.hpp"
#include "messages.pb.h"

namespace neuro {
namespace ledger {

const std::string MAIN_BRANCH_NAME =
    messages::Branch_Name(messages::Branch::MAIN);
const std::string FORK_BRANCH_NAME =
    messages::Branch_Name(messages::Branch::FORK);

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
  std::lock_guard<std::mutex> lock(_ledger_mutex);
  messages::Block block0;
  if (unsafe_get_block(0, &block0)) {
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
  tagged_block0.mutable_branch_path()->add_branch_ids(0);
  tagged_block0.mutable_branch_path()->add_block_numbers(0);
  *tagged_block0.mutable_block() = block0;
  unsafe_insert_block(&tagged_block0);

  //! add index to mongo collection
  _blocks.create_index(bss::document{} << "block.header.id" << 1
                                       << bss::finalize);
  _blocks.create_index(bss::document{} << "block.header.height" << 1
                                       << bss::finalize);
  _blocks.create_index(bss::document{} << "block.header.previousBlockHash" << 1
                                       << bss::finalize);
  _blocks.create_index(bss::document{} << "branchPath.branchIds.0" << 1
                                       << bss::finalize);
  _blocks.create_index(bss::document{} << "block.score" << 1 << bss::finalize);
  _transactions.create_index(bss::document{} << "transaction.id" << 1
                                             << bss::finalize);
  _transactions.create_index(bss::document{} << "blockId" << 1
                                             << bss::finalize);
  _transactions.create_index(bss::document{}
                             << "transaction.outputs.address.data" << 1
                             << bss::finalize);
  return true;
}

void LedgerMongodb::remove_all() {
  std::lock_guard<std::mutex> lock(_ledger_mutex);
  _blocks.delete_many(bss::document{} << bss::finalize);
  _transactions.delete_many(bss::document{} << bss::finalize);
}

messages::BlockHeight LedgerMongodb::height() const {
  std::lock_guard<std::mutex> lock(_ledger_mutex);
  auto query = bss::document{} << "branch" << MAIN_BRANCH_NAME << bss::finalize;

  auto options = projection("block.header.height");
  options.sort(bss::document{} << "block.header.height" << -1 << bss::finalize);

  const auto result = _blocks.find_one(std::move(query), options);
  if (!result) {
    return 0;
  }

  return result->view()["block"]["header"]["height"].get_int32().value;
}

bool LedgerMongodb::is_ancestor(const messages::TaggedBlock &ancestor,
                                const messages::TaggedBlock &block) const {
  if (!ancestor.has_branch_path() || !block.has_branch_path()) {
    return false;
  }
  auto block_path = block.branch_path();
  auto ancestor_path = ancestor.branch_path();
  auto ancestor_size = ancestor_path.branch_ids_size();
  auto block_size = block_path.branch_ids_size();
  assert(block_path.block_numbers_size() == block_size);
  assert(ancestor_path.block_numbers_size() == ancestor_size);
  if (ancestor_size == 0 || ancestor_size > block_size) {
    return false;
  }

  // A block A is the ancestor of a block B if
  //     - the branch_ids of B ends with the branch_ids of A
  // and - the block_numbers of B ends with (the block_numbers of A without the
  //       first element) and
  // and - the block number of B (bnB) corresponding to the first
  //       block number of A (bnA) obeys bnB >= bnA
  for (int i = 0; i < ancestor_size; i++) {
    if (ancestor_path.branch_ids(ancestor_size - 1 - i) !=
        block_path.branch_ids(block_size - 1 - i)) {
      return false;
    }
    auto ancestor_block_number =
        ancestor_path.block_numbers(ancestor_size - 1 - i);
    auto block_number = block_path.block_numbers(block_size - 1 - i);
    if (i != ancestor_size - 1) {
      if (block_number != ancestor_block_number) {
        return false;
      }
    } else {
      return ancestor_block_number <= block_number;
    }
  }

  // You should never arrive here
  assert(false);
  return false;
}

bool LedgerMongodb::is_main_branch(
    const messages::TaggedTransaction &tagged_transaction) const {
  messages::TaggedBlock tagged_block;
  return unsafe_get_block(tagged_transaction.block_id(), &tagged_block) &&
         tagged_block.branch() == messages::Branch::MAIN;
}

bool LedgerMongodb::get_block_header(const messages::BlockID &id,
                                     messages::BlockHeader *header) const {
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

bool LedgerMongodb::get_last_block_header(
    messages::BlockHeader *block_header) const {
  std::lock_guard<std::mutex> lock(_ledger_mutex);
  auto query = bss::document{} << "branch" << MAIN_BRANCH_NAME << bss::finalize;
  auto options = projection("block.header");
  options.sort(bss::document{} << "block.header.height" << -1 << bss::finalize);

  const auto result = _blocks.find_one(std::move(query), options);
  if (!result) {
    return false;
  }

  messages::from_bson(result->view()["block"]["header"].get_document(),
                      block_header);
  return true;
}

int LedgerMongodb::fill_block_transactions(messages::Block *block) const {
  assert(block->transactions().size() == 0);
  auto query = bss::document{} << "blockId" << to_bson(block->header().id())
                               << bss::finalize;
  auto options = projection("transaction");
  options.sort(bss::document{} << "transaction.id" << 1 << bss::finalize);
  auto cursor = _transactions.find(std::move(query), options);

  int num_transactions = 0;
  for (const auto &bson_transaction : cursor) {
    num_transactions++;
    auto transaction = block->add_transactions();
    from_bson(bson_transaction["transaction"].get_document(), transaction);
  }

  return num_transactions;
}

bool LedgerMongodb::unsafe_get_block(const messages::BlockID &id,
                                     messages::TaggedBlock *tagged_block,
                                     bool include_transactions) const {
  auto query = bss::document{} << "block.header.id" << to_bson(id)
                               << bss::finalize;
  auto result = _blocks.find_one(std::move(query), remove_OID());
  if (!result) {
    return false;
  }
  messages::from_bson(result->view(), tagged_block);
  if (include_transactions) {
    fill_block_transactions(tagged_block->mutable_block());
  }
  return true;
}

bool LedgerMongodb::get_block(const messages::BlockID &id,
                              messages::TaggedBlock *tagged_block) const {
  std::lock_guard<std::mutex> lock(_ledger_mutex);
  return unsafe_get_block(id, tagged_block);
}

bool LedgerMongodb::get_block(const messages::BlockID &id,
                              messages::Block *block) const {
  std::lock_guard<std::mutex> lock(_ledger_mutex);
  auto query = bss::document{} << "block.header.id" << to_bson(id)
                               << bss::finalize;
  auto result = _blocks.find_one(std::move(query), projection("block"));
  if (!result) {
    return false;
  }
  messages::from_bson(result->view()["block"].get_document(), block);
  fill_block_transactions(block);
  return true;
}

bool LedgerMongodb::unsafe_get_block_by_previd(
    const messages::BlockID &previd, messages::Block *block,
    bool include_transactions) const {
  // Look for a block in the main branch.
  // There may be several blocks with the same previd in forks.
  auto query = bss::document{} << "branch" << MAIN_BRANCH_NAME
                               << "block.header.previousBlockHash"
                               << to_bson(previd) << bss::finalize;

  const auto result = _blocks.find_one(std::move(query), projection("block"));

  if (!result) {
    return false;
  }

  messages::from_bson(result->view()["block"].get_document(), block);
  if (include_transactions) {
    fill_block_transactions(block);
  }
  return true;
}

bool LedgerMongodb::get_block_by_previd(const messages::BlockID &previd,
                                        messages::Block *block,
                                        bool include_transactions) const {
  std::lock_guard<std::mutex> lock(_ledger_mutex);
  return unsafe_get_block_by_previd(previd, block, include_transactions);
}

bool LedgerMongodb::unsafe_get_blocks_by_previd(
    const messages::BlockID &previd,
    std::vector<messages::TaggedBlock> *tagged_blocks,
    bool include_transactions) const {
  auto query = bss::document{} << "block.header.previousBlockHash"
                               << to_bson(previd) << bss::finalize;

  auto bson_blocks = _blocks.find(std::move(query), remove_OID());

  if (bson_blocks.begin() == bson_blocks.end()) {
    return false;
  }

  for (const auto &bson_block : bson_blocks) {
    messages::TaggedBlock tagged_block;
    // auto tagged_block = tagged_blocks->emplace_back();
    // TODO this crashed and I don't know why
    messages::from_bson(bson_block, &tagged_block);
    tagged_blocks->push_back(tagged_block);
    if (include_transactions) {
      fill_block_transactions(tagged_block.mutable_block());
    }
  }

  return true;
}

bool LedgerMongodb::get_blocks_by_previd(
    const messages::BlockID &previd,
    std::vector<messages::TaggedBlock> *tagged_blocks,
    bool include_transactions) const {
  std::lock_guard<std::mutex> lock(_ledger_mutex);
  return unsafe_get_blocks_by_previd(previd, tagged_blocks,
                                     include_transactions);
}

bool LedgerMongodb::unsafe_get_block(const messages::BlockHeight height,
                                     messages::Block *block,
                                     bool include_transactions) const {
  auto query = bss::document{} << "branch" << MAIN_BRANCH_NAME
                               << "block.header.height" << height
                               << bss::finalize;

  const auto result = _blocks.find_one(std::move(query), projection("block"));
  if (!result) {
    return false;
  }

  messages::from_bson(result->view()["block"].get_document(), block);
  if (include_transactions) {
    fill_block_transactions(block);
  }
  return true;
}

bool LedgerMongodb::get_block(const messages::BlockHeight height,
                              messages::Block *block,
                              bool include_transactions) const {
  std::lock_guard<std::mutex> lock(_ledger_mutex);
  return unsafe_get_block(height, block, include_transactions);
}

bool LedgerMongodb::unsafe_get_block(const messages::BlockHeight height,
                                     messages::TaggedBlock *tagged_block,
                                     bool include_transactions) const {
  auto query = bss::document{} << "branch" << MAIN_BRANCH_NAME
                               << "block.header.height" << height
                               << bss::finalize;

  const auto result = _blocks.find_one(std::move(query), remove_OID());
  if (!result) {
    return false;
  }

  messages::from_bson(result->view(), tagged_block);
  if (include_transactions) {
    fill_block_transactions(tagged_block->mutable_block());
  }
  return true;
}

bool LedgerMongodb::get_block(const messages::BlockHeight height,
                              messages::TaggedBlock *tagged_block,
                              bool include_transactions) const {
  std::lock_guard<std::mutex> lock(_ledger_mutex);
  return unsafe_get_block(height, tagged_block, include_transactions);
}

bool LedgerMongodb::unsafe_insert_block(messages::TaggedBlock *tagged_block) {
  messages::TaggedBlock unused;
  if (unsafe_get_block(tagged_block->block().header().id(), &unused)) {
    // The block already exists
    return false;
  }

  const auto header = tagged_block->block().header();
  auto bson_header = messages::to_bson(header);

  std::vector<bsoncxx::document::value> bson_transactions;
  for (const auto &transaction : tagged_block->block().transactions()) {
    messages::TaggedTransaction tagged_transaction;
    *tagged_transaction.mutable_transaction() = transaction;
    *tagged_transaction.mutable_block_id() = header.id();
    bson_transactions.push_back(messages::to_bson(tagged_transaction));
    if (tagged_block->branch() == messages::Branch::MAIN) {
      auto query = bss::document{}
                   << "transaction.id" << messages::to_bson(transaction.id())
                   << "blockId" << bsoncxx::types::b_null{} << bss::finalize;
      _transactions.delete_one(std::move(query));
    }
  }
  tagged_block->mutable_block()->clear_transactions();
  auto bson_block = messages::to_bson(*tagged_block);
  auto result = _blocks.insert_one(std::move(bson_block));
  if (!result) {
    return false;
  }
  if (bson_transactions.size() > 0) {
    return (bool)_transactions.insert_many(std::move(bson_transactions));
  }
  return true;
}

bool LedgerMongodb::insert_block(messages::TaggedBlock *tagged_block) {
  std::lock_guard<std::mutex> lock(_ledger_mutex);
  return unsafe_insert_block(tagged_block);
}

bool LedgerMongodb::delete_block(const messages::Hash &id) {
  std::lock_guard<std::mutex> lock(_ledger_mutex);
  auto delete_block_query = bss::document{} << "block.header.id"
                                            << messages::to_bson(id)
                                            << bss::finalize;
  auto result = _blocks.delete_one(std::move(delete_block_query));
  bool did_delete = result && result->deleted_count() > 0;
  if (did_delete) {
    auto delete_transaction_query =
        bss::document{} << "blockId" << messages::to_bson(id) << bss::finalize;
    auto res_transaction =
        _transactions.delete_many(std::move(delete_transaction_query));
  }
  return did_delete;
}

bool LedgerMongodb::get_transaction(const messages::Hash &id,
                                    messages::Transaction *transaction) const {
  messages::BlockHeight block_height;
  return get_transaction(id, transaction, &block_height);
}

bool LedgerMongodb::get_transaction(const messages::Hash &id,
                                    messages::Transaction *transaction,
                                    messages::BlockHeight *blockheight) const {
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
    if (!tagged_transaction.has_block_id() ||
        (unsafe_get_block(tagged_transaction.block_id(), &tagged_block) &&
         tagged_block.branch() == messages::Branch::MAIN)) {
      *transaction = tagged_transaction.transaction();
      *blockheight = tagged_block.block().header().height();
      return true;
    }
  }
  return false;
}

std::size_t LedgerMongodb::total_nb_transactions() const {
  std::lock_guard<std::mutex> lock(_ledger_mutex);
  auto lookup = bss::document{} << "from"
                                << "blocks"
                                << "foreignField"
                                << "block.header.id"
                                << "localField"
                                << "blockId"
                                << "as"
                                << "tagged_block" << bss::finalize;

  auto match = bss::document{} << "tagged_block.branch"
                               << "MAIN" << bss::finalize;

  auto group = bss::document{} << "_id" << bsoncxx::types::b_null{} << "count"
                               << bss::open_document << "$sum" << 1
                               << bss::close_document << bss::finalize;

  mongocxx::pipeline pipeline;
  pipeline.lookup(lookup.view());
  pipeline.match(match.view());
  pipeline.group(group.view());
  auto cursor = _transactions.aggregate(pipeline);
  return (*cursor.begin())["count"].get_int32();
}

std::size_t LedgerMongodb::total_nb_blocks() const {
  std::lock_guard<std::mutex> lock(_ledger_mutex);
  auto query = bss::document{} << "branch" << MAIN_BRANCH_NAME << bss::finalize;
  return _blocks.count(std::move(query));
}

bool LedgerMongodb::for_each(const Filter &filter, Functor functor) const {
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

  auto bson_transactions =
      _transactions.find((query << bss::finalize).view(), remove_OID());

  bool applied_functor = false;
  for (const auto &bson_transaction : bson_transactions) {
    messages::TaggedTransaction tagged_transaction;
    from_bson(bson_transaction, &tagged_transaction);
    if (!tagged_transaction.has_block_id() ||
        is_main_branch(tagged_transaction)) {
      functor(tagged_transaction.transaction());
      applied_functor = true;
    }
  }

  return applied_functor;
}

messages::BranchID LedgerMongodb::new_branch_id() const {
  auto query = bss::document{} << bss::finalize;
  mongocxx::options::find find_options;

  auto projection_doc = bss::document{} << "_id" << 0 << "branchPath.branchIds"
                                        << 1 << bss::finalize;

  find_options.projection(std::move(projection_doc));
  find_options.sort(bss::document{} << "branchPath.branchIds.0" << -1
                                    << bss::finalize);

  auto max_branch_id = _blocks.find_one(std::move(query), find_options);
  return max_branch_id->view()["branchPath"]["branchIds"][0].get_int32() + 1;
}

bool LedgerMongodb::add_transaction(
    const messages::TaggedTransaction &tagged_transaction) {
  std::lock_guard<std::mutex> lock(_ledger_mutex);
  auto bson_transaction = messages::to_bson(tagged_transaction);
  auto result = _transactions.insert_one(std::move(bson_transaction));
  if (result) {
    return true;
  }
  LOG_INFO << "Failed to insert transaction " << tagged_transaction;
  return false;
}

bool LedgerMongodb::add_to_transaction_pool(
    const messages::Transaction &transaction) {
  messages::TaggedTransaction tagged_transaction;
  tagged_transaction.mutable_transaction()->CopyFrom(transaction);
  return add_transaction(tagged_transaction);
}

bool LedgerMongodb::delete_transaction(const messages::Hash &id) {
  // Delete a transaction in the transaction pool
  std::lock_guard<std::mutex> lock(_ledger_mutex);
  auto query = bss::document{} << "transaction.id" << messages::to_bson(id)
                               << "blockId" << bsoncxx::types::b_null{}
                               << bss::finalize;
  auto result = _transactions.delete_one(std::move(query));
  bool did_delete = result && result->deleted_count();
  if (did_delete) {
    return true;
  }
  LOG_INFO << "Failed to delete transaction with id " << id;
  return did_delete;
}

std::size_t LedgerMongodb::get_transaction_pool(messages::Block *block) {
  // This method put the whole transaction pool in a block but does not cleanup
  // the transaction pool.
  // TODO add a way to limit the number of transactions you want to include
  std::lock_guard<std::mutex> lock(_ledger_mutex);
  auto query = bss::document{} << "blockId" << bsoncxx::types::b_null{}
                               << bss::finalize;

  auto options = projection("transaction");
  options.sort(bss::document{} << "transaction.id" << 1 << bss::finalize);
  auto cursor = _transactions.find(std::move(query), options);

  int num_transactions = 0;
  for (const auto &bson_transaction : cursor) {
    num_transactions++;
    auto transaction = block->add_transactions();
    from_bson(bson_transaction["transaction"].get_document(), transaction);
  }

  return num_transactions;
}

bool LedgerMongodb::insert_block(messages::Block *block) {
  std::lock_guard<std::mutex> lock(_ledger_mutex);
  messages::TaggedBlock tagged_block;
  tagged_block.mutable_block()->CopyFrom(*block);
  tagged_block.set_branch(messages::Branch::DETACHED);
  return unsafe_insert_block(&tagged_block) &&
         set_branch_path(tagged_block.block().header());
}

messages::BranchPath LedgerMongodb::unsafe_fork_from(
    const messages::BranchPath &branch_path) const {
  // We must add the new branch_id at the beginning of the repeated field
  // And sadly protobuf does not support that
  messages::BranchPath new_branch_path;
  new_branch_path.add_branch_ids(new_branch_id());
  new_branch_path.add_block_numbers(0);
  for (const auto &branch_id : branch_path.branch_ids()) {
    new_branch_path.add_branch_ids(branch_id);
  }
  for (const auto &block_number : branch_path.block_numbers()) {
    new_branch_path.add_block_numbers(block_number);
  }
  return new_branch_path;
}

messages::BranchPath LedgerMongodb::fork_from(
    const messages::BranchPath &branch_path) const {
  std::lock_guard<std::mutex> lock(_ledger_mutex);
  return unsafe_fork_from(branch_path);
}

messages::BranchPath LedgerMongodb::unsafe_first_child(
    const messages::BranchPath &branch_path) const {
  auto new_branch_path = messages::BranchPath(branch_path);
  *new_branch_path.mutable_block_numbers()->Mutable(0) =
      branch_path.block_numbers(0) + 1;
  return new_branch_path;
}

messages::BranchPath LedgerMongodb::first_child(
    const messages::BranchPath &branch_path) const {
  // The mutex is not really useful here but all public method should have a
  // mutex
  std::lock_guard<std::mutex> lock(_ledger_mutex);
  return unsafe_first_child(branch_path);
}

bool LedgerMongodb::set_branch_path(const messages::BlockHeader &block_header,
                                    const messages::BranchPath &branch_path) {
  // Set the branch path of the given block
  auto filter = bss::document{} << "block.header.id"
                                << to_bson(block_header.id()) << bss::finalize;
  auto update = bss::document{} << "$set" << bss::open_document << "branch_path"
                                << messages::to_bson(branch_path) << "branch"
                                << FORK_BRANCH_NAME << bss::close_document
                                << bss::finalize;
  auto update_result = _blocks.update_one(std::move(filter), std::move(update));
  if (!(update_result && update_result->modified_count() > 0)) {
    return false;
  }

  // Set the branch path of the children
  std::vector<messages::TaggedBlock> tagged_blocks;
  unsafe_get_blocks_by_previd(block_header.id(), &tagged_blocks, false);
  for (uint32_t i = 0; i < tagged_blocks.size(); i++) {
    const auto tagged_block = tagged_blocks.at(i);
    assert(!tagged_block.has_branch_path());
    assert(tagged_block.branch() == messages::Branch::DETACHED);
    if (i == 0) {
      if (!set_branch_path(tagged_block.block().header(),
                           unsafe_first_child(branch_path))) {
        return false;
      }
    } else {
      // If there are several children there is a fork which needs a new branch
      // ID
      if (!set_branch_path(tagged_block.block().header(),
                           unsafe_fork_from(branch_path))) {
        return false;
      }
    }
  }
  return true;
}

bool LedgerMongodb::set_branch_path(const messages::BlockHeader &block_header) {
  // Set the branch path of a block depending on the branch path of its parent
  messages::TaggedBlock parent;

  if (!unsafe_get_block(block_header.previous_block_hash(), &parent, false) ||
      parent.branch() == messages::Branch::DETACHED) {
    // If the parent does not have a branch path it is a detached block and
    // there is nothing to do
    return true;
  }

  std::vector<messages::TaggedBlock> children;
  unsafe_get_blocks_by_previd(block_header.previous_block_hash(), &children,
                              false);

  // If our parent has more than one child it means the current block is a
  // fork and needs a new branch ID
  const auto branch_path = children.size() > 1
                               ? unsafe_fork_from(parent.branch_path())
                               : unsafe_first_child(parent.branch_path());
  return set_branch_path(block_header, branch_path);
};

bool LedgerMongodb::get_unscored_forks(
    std::vector<messages::TaggedBlock> *tagged_blocks,
    bool include_transactions) const {
  std::lock_guard<std::mutex> lock(_ledger_mutex);
  auto query = bss::document{} << "branch" << FORK_BRANCH_NAME << "score"
                               << bsoncxx::types::b_null{} << bss::finalize;
  auto result = _blocks.find(std::move(query), remove_OID());
  for (const auto &bson_block : result) {
    auto &tagged_block = tagged_blocks->emplace_back();
    from_bson(bson_block, &tagged_block);
    if (include_transactions) {
      fill_block_transactions(tagged_block.mutable_block());
    }
  }
  return true;
}

bool LedgerMongodb::set_block_score(const messages::Hash &id,
                                    messages::BlockScore score) {
  std::lock_guard<std::mutex> lock(_ledger_mutex);
  auto filter = bss::document{} << "block.header.id" << to_bson(id)
                                << bss::finalize;
  auto update = bss::document{} << "$set" << bss::open_document << "score"
                                << score << bss::close_document
                                << bss::finalize;
  auto update_result = _blocks.update_one(std::move(filter), std::move(update));
  if (!(update_result && update_result->modified_count() > 0)) {
    return false;
  }
  return true;
}

bool LedgerMongodb::update_branch_tag(const messages::Hash &id,
                                      const messages::Branch &branch) {
  auto filter = bss::document{} << "block.header.id" << to_bson(id)
                                << bss::finalize;
  auto update = bss::document{} << "$set" << bss::open_document << "branch"
                                << messages::Branch_Name(branch)
                                << bss::close_document << bss::finalize;
  auto update_result = _blocks.update_one(std::move(filter), std::move(update));
  if (!(update_result && update_result->modified_count() > 0)) {
    return false;
  }
  return true;
}

bool LedgerMongodb::update_main_branch(messages::TaggedBlock *main_branch_tip) {
  std::lock_guard<std::mutex> lock(_ledger_mutex);
  auto query = bss::document{} << bss::finalize;
  auto options = remove_OID();
  options.sort(bss::document{} << "score" << -1 << bss::finalize);
  auto bson_block = _blocks.find_one(std::move(query), options);
  if (!bson_block) {
    return false;
  }
  messages::from_bson(bson_block->view(), main_branch_tip);

  assert(main_branch_tip->branch() != messages::Branch::DETACHED);
  if (main_branch_tip->branch() == messages::Branch::MAIN) {
    return true;
  }

  // Go back from the new tip to a block tagged MAIN
  std::vector<messages::Hash> new_main_branch;
  messages::TaggedBlock tagged_block{*main_branch_tip};
  do {
    new_main_branch.push_back(tagged_block.block().header().id());
    assert(unsafe_get_block(tagged_block.block().header().previous_block_hash(),
                            &tagged_block, false));
  } while (tagged_block.branch() != messages::Branch::MAIN);

  // Go up from this MAIN block to the previous MAIN tip
  std::vector<messages::Hash> previous_main_branch;
  messages::Block block{tagged_block.block()};
  while (unsafe_get_block_by_previd(block.header().id(), &block)) {
    previous_main_branch.push_back(block.header().id());
  }

  // This order makes the database never be in an inconsistent state
  std::reverse(previous_main_branch.begin(), previous_main_branch.end());
  for (const auto &id : previous_main_branch) {
    if (!update_branch_tag(id, messages::Branch::FORK)) {
      return false;
    }
  }

  // This order makes the database never be in a inconsistent state
  std::reverse(new_main_branch.begin(), new_main_branch.end());
  for (const auto &id : new_main_branch) {
    if (!update_branch_tag(id, messages::Branch::MAIN)) {
      return false;
    }
  }

  // In my code I trust, update_branch_tag modified the branch of the tip
  main_branch_tip->set_branch(messages::Branch::MAIN);

  return true;
}

void LedgerMongodb::empty_database() { _db.drop(); }

}  // namespace ledger
}  // namespace neuro
