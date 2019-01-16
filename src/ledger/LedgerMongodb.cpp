#include "ledger/LedgerMongodb.hpp"
#include "common/logger.hpp"
#include "messages.pb.h"

namespace neuro {
namespace ledger {

const std::string MAIN_BRANCH_NAME =
    messages::Branch_Name(messages::Branch::MAIN);
const std::string FORK_BRANCH_NAME =
    messages::Branch_Name(messages::Branch::FORK);
const std::string UNVERIFIED_BRANCH_NAME =
    messages::Branch_Name(messages::Branch::UNVERIFIED);
const std::string _ID = "_id";
const std::string $SET = "$set";
const std::string AS = "as";
const std::string ASSEMBLIES = "assemblies";
const std::string ASSEMBLY_ID = "assemblyId";
const std::string ADDRESS = "address";
const std::string BLOCK = "block";
const std::string BLOCK_ID = "blockId";
const std::string BLOCKS = "blocks";
const std::string BLOCK_HEADER_ID = "block.header.id";
const std::string BLOCK_HEADER_HEIGHT = "block.header.height";
const std::string BLOCK_HEADER_PREVIOUS_BLOCK_HASH =
    "block.header.previousBlockHash";
const std::string BRANCH = "branch";
const std::string BRANCH_IDS = "branchIds";
const std::string BRANCH_PATH = "branchPath";
const std::string COUNT = "count";
const std::string DATA = "data";
const std::string FINISHED_COMPUTATION = "finishedComputation";
const std::string FROM = "from";
const std::string FOREIGN_FIELD = "foreignField";
const std::string HEADER = "header";
const std::string HEIGHT = "height";
const std::string ID = "id";
const std::string INPUTS = "inputs";
const std::string LOCAL_FIELD = "localField";
const std::string MAIN = "MAIN";
const std::string NB_ADDRESSES = "nbAddresses";
const std::string PREVIOUS_ASSEMBLY_ID = "previousAssemblyId";
const std::string OUTPUTS = "outputs";
const std::string OUTPUT_ID = "outputId";
const std::string PII = "pii";
const std::string RANK = "rank";
const std::string SCORE = "score";
const std::string SEED = "seed";
const std::string TAGGED_BLOCK = "taggedBlock";
const std::string TRANSACTION_ID = "transaction.id";
const std::string TRANSACTION = "transaction";
const std::string TRANSACTIONS = "transactions";

mongocxx::instance LedgerMongodb::_instance{};

LedgerMongodb::LedgerMongodb(const std::string &url, const std::string &db_name)
    : _uri(url),
      _client(_uri),
      _db(_client[db_name]),
      _blocks(_db.collection(BLOCKS)),
      _transactions(_db.collection(TRANSACTIONS)),
      _pii(_db.collection(PII)),
      _assemblies(_db.collection(ASSEMBLIES)) {}

LedgerMongodb::LedgerMongodb(const messages::config::Database &config)
    : LedgerMongodb(config.url(), config.db_name()) {
  init_block0(config);
  set_main_branch_tip();
}

mongocxx::options::find LedgerMongodb::remove_OID() const {
  mongocxx::options::find find_options;
  auto projection_doc = bss::document{} << _ID << 0
                                        << bss::finalize;  // remove _id
  find_options.projection(std::move(projection_doc));
  return find_options;
}

mongocxx::options::find LedgerMongodb::projection(
    const std::string &field) const {
  mongocxx::options::find find_options;
  auto projection_doc = bss::document{} << _ID << 0 << field << 1
                                        << bss::finalize;
  find_options.projection(std::move(projection_doc));
  return find_options;
}

mongocxx::options::find LedgerMongodb::projection(
    const std::string &field0, const std::string &field1) const {
  mongocxx::options::find find_options;
  auto projection_transaction = bss::document{} << _ID << 0 << field0 << 1
                                                << field1 << 1 << bss::finalize;
  find_options.projection(std::move(projection_transaction));
  return find_options;
}

bool LedgerMongodb::init_block0(const messages::config::Database &config) {
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
      from_bson(d.view(), &block0);
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
  _blocks.create_index(bss::document{} << BLOCK_HEADER_ID << 1
                                       << bss::finalize);
  _blocks.create_index(bss::document{} << BLOCK_HEADER_HEIGHT << 1
                                       << bss::finalize);
  _blocks.create_index(bss::document{} << BLOCK_HEADER_PREVIOUS_BLOCK_HASH << 1
                                       << bss::finalize);
  _blocks.create_index(bss::document{} << BRANCH_PATH + "." + BRANCH_IDS + ".0"
                                       << 1 << bss::finalize);
  _blocks.create_index(bss::document{} << BLOCK + "." + SCORE << 1
                                       << bss::finalize);
  _transactions.create_index(bss::document{} << TRANSACTION_ID << 1
                                             << bss::finalize);
  _transactions.create_index(bss::document{} << BLOCK_ID << 1 << bss::finalize);
  _transactions.create_index(bss::document{} << TRANSACTION + "." + OUTPUTS +
                                                    "." + ADDRESS + "." + DATA
                                             << 1 << bss::finalize);
  _pii.create_index(bss::document{} << ADDRESS << 1 << ASSEMBLY_ID << 1
                                    << bss::finalize);
  _pii.create_index(bss::document{} << RANK << 1 << ASSEMBLY_ID << 1
                                    << bss::finalize);
  _assemblies.create_index(bss::document{} << ASSEMBLY_ID << 1
                                           << bss::finalize);
  _assemblies.create_index(bss::document{} << PREVIOUS_ASSEMBLY_ID << 1
                                           << bss::finalize);
  _assemblies.create_index(bss::document{} << FINISHED_COMPUTATION << 1
                                           << bss::finalize);

  return true;
}

void LedgerMongodb::remove_all() {
  std::lock_guard<std::mutex> lock(_ledger_mutex);
  _blocks.delete_many(bss::document{} << bss::finalize);
  _transactions.delete_many(bss::document{} << bss::finalize);
}

messages::TaggedBlock LedgerMongodb::get_main_branch_tip() const {
  return _main_branch_tip;
}

bool LedgerMongodb::set_main_branch_tip() {
  std::lock_guard<std::mutex> lock(_ledger_mutex);
  return unsafe_get_block(unsafe_height(), &_main_branch_tip, false);
}

messages::BlockHeight LedgerMongodb::unsafe_height() const {
  auto query = bss::document{} << BRANCH << MAIN_BRANCH_NAME << bss::finalize;

  auto options = projection(BLOCK_HEADER_HEIGHT);
  options.sort(bss::document{} << BLOCK_HEADER_HEIGHT << -1 << bss::finalize);

  const auto result = _blocks.find_one(std::move(query), options);
  if (!result) {
    return 0;
  }

  return result->view()[BLOCK][HEADER][HEIGHT].get_int32().value;
}

messages::BlockHeight LedgerMongodb::height() const {
  std::lock_guard<std::mutex> lock(_ledger_mutex);
  return unsafe_height();
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
  auto query = bss::document{} << BLOCK_HEADER_ID << to_bson(id)
                               << bss::finalize;
  auto result =
      _blocks.find_one(std::move(query), projection(BLOCK + "." + HEADER));
  if (!result) {
    return false;
  }
  from_bson(result->view()[BLOCK][HEADER].get_document(), header);
  return true;
}

bool LedgerMongodb::get_last_block_header(
    messages::BlockHeader *block_header) const {
  std::lock_guard<std::mutex> lock(_ledger_mutex);
  auto query = bss::document{} << BRANCH << MAIN_BRANCH_NAME << bss::finalize;
  auto options = projection(BLOCK + "." + HEADER);
  options.sort(bss::document{} << BLOCK_HEADER_HEIGHT << -1 << bss::finalize);

  const auto result = _blocks.find_one(std::move(query), options);
  if (!result) {
    return false;
  }

  from_bson(result->view()[BLOCK][HEADER].get_document(), block_header);
  return true;
}

int LedgerMongodb::fill_block_transactions(messages::Block *block) const {
  assert(block->transactions().size() == 0);
  auto query = bss::document{} << BLOCK_ID << to_bson(block->header().id())
                               << bss::finalize;
  auto options = remove_OID();
  options.sort(bss::document{} << TRANSACTION_ID << 1 << bss::finalize);
  auto cursor = _transactions.find(std::move(query), options);

  int num_transactions = 0;
  for (const auto &bson_transaction : cursor) {
    num_transactions++;
    messages::TaggedTransaction tagged_transaction;
    from_bson(bson_transaction, &tagged_transaction);

    auto transaction = tagged_transaction.is_coinbase()
                           ? block->add_coinbases()
                           : block->add_transactions();
    transaction->CopyFrom(tagged_transaction.transaction());
  }

  return num_transactions;
}

bool LedgerMongodb::unsafe_get_block(const messages::BlockID &id,
                                     messages::TaggedBlock *tagged_block,
                                     bool include_transactions) const {
  auto query = bss::document{} << BLOCK_HEADER_ID << to_bson(id)
                               << bss::finalize;
  auto result = _blocks.find_one(std::move(query), remove_OID());
  if (!result) {
    return false;
  }

  from_bson(result->view(), tagged_block);

  if (include_transactions) {
    fill_block_transactions(tagged_block->mutable_block());
  }

  return true;
}

bool LedgerMongodb::get_block(const messages::BlockID &id,
                              messages::TaggedBlock *tagged_block,
                              bool include_transactions) const {
  std::lock_guard<std::mutex> lock(_ledger_mutex);
  return unsafe_get_block(id, tagged_block, include_transactions);
}

bool LedgerMongodb::get_block(const messages::BlockID &id,
                              messages::Block *block,
                              bool include_transactions) const {
  std::lock_guard<std::mutex> lock(_ledger_mutex);
  auto query = bss::document{} << BLOCK_HEADER_ID << to_bson(id)
                               << bss::finalize;
  auto result = _blocks.find_one(std::move(query), projection(BLOCK));
  if (!result) {
    return false;
  }
  from_bson(result->view()[BLOCK].get_document(), block);
  fill_block_transactions(block);
  return true;
}

bool LedgerMongodb::unsafe_get_block_by_previd(
    const messages::BlockID &previd, messages::Block *block,
    bool include_transactions) const {
  // Look for a block in the main branch.
  // There may be several blocks with the same previd in forks.
  auto query = bss::document{} << BRANCH << MAIN_BRANCH_NAME
                               << BLOCK_HEADER_PREVIOUS_BLOCK_HASH
                               << to_bson(previd) << bss::finalize;

  const auto result = _blocks.find_one(std::move(query), projection(BLOCK));

  if (!result) {
    return false;
  }

  from_bson(result->view()[BLOCK].get_document(), block);
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
  auto query = bss::document{} << BLOCK_HEADER_PREVIOUS_BLOCK_HASH
                               << to_bson(previd) << bss::finalize;

  auto bson_blocks = _blocks.find(std::move(query), remove_OID());

  if (bson_blocks.begin() == bson_blocks.end()) {
    return false;
  }

  for (const auto &bson_block : bson_blocks) {
    messages::TaggedBlock tagged_block;
    // auto tagged_block = tagged_blocks->emplace_back();
    // TODO this crashed and I don't know why
    from_bson(bson_block, &tagged_block);
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
  auto query = bss::document{} << BRANCH << MAIN_BRANCH_NAME
                               << BLOCK_HEADER_HEIGHT << height
                               << bss::finalize;

  const auto result = _blocks.find_one(std::move(query), projection(BLOCK));
  if (!result) {
    return false;
  }

  from_bson(result->view()[BLOCK].get_document(), block);
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
  auto query = bss::document{} << BRANCH << MAIN_BRANCH_NAME
                               << BLOCK_HEADER_HEIGHT << height
                               << bss::finalize;

  const auto result = _blocks.find_one(std::move(query), remove_OID());
  if (!result) {
    return false;
  }

  from_bson(result->view(), tagged_block);
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
  auto bson_header = to_bson(header);

  std::vector<bsoncxx::document::value> bson_transactions;

  for (const auto &transaction : tagged_block->block().transactions()) {
    messages::TaggedTransaction tagged_transaction;
    tagged_transaction.set_is_coinbase(false);
    tagged_transaction.mutable_transaction()->CopyFrom(transaction);
    tagged_transaction.mutable_block_id()->CopyFrom(header.id());
    bson_transactions.push_back(to_bson(tagged_transaction));
    if (tagged_block->branch() == messages::Branch::MAIN) {
      auto query = bss::document{} << TRANSACTION_ID
                                   << to_bson(transaction.id()) << BLOCK_ID
                                   << bsoncxx::types::b_null{} << bss::finalize;
      _transactions.delete_one(std::move(query));
    }
  }

  for (const auto &transaction : tagged_block->block().coinbases()) {
    messages::TaggedTransaction tagged_transaction;
    tagged_transaction.set_is_coinbase(true);
    tagged_transaction.mutable_transaction()->CopyFrom(transaction);
    tagged_transaction.mutable_block_id()->CopyFrom(header.id());
    bson_transactions.push_back(to_bson(tagged_transaction));
  }

  tagged_block->mutable_block()->clear_transactions();
  tagged_block->mutable_block()->clear_coinbases();
  auto bson_block = to_bson(*tagged_block);
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
  auto delete_block_query = bss::document{} << BLOCK_HEADER_ID << to_bson(id)
                                            << bss::finalize;
  auto result = _blocks.delete_one(std::move(delete_block_query));
  bool did_delete = result && result->deleted_count() > 0;
  if (did_delete) {
    auto delete_transaction_query = bss::document{} << BLOCK_ID << to_bson(id)
                                                    << bss::finalize;
    auto res_transaction =
        _transactions.delete_many(std::move(delete_transaction_query));
  }
  return did_delete;
}

bool LedgerMongodb::delete_block_and_children(const messages::Hash &id) {
  std::vector<messages::TaggedBlock> tagged_blocks;
  get_blocks_by_previd(id, &tagged_blocks);
  bool result = true;
  for (auto tagged_block : tagged_blocks) {
    result &= delete_block_and_children(tagged_block.block().header().id());
  }
  if (result) {
    result &= delete_block(id);
  }
  return result;
}

bool LedgerMongodb::get_transaction(
    const messages::Hash &id, messages::TaggedTransaction *tagged_transaction,
    const messages::TaggedBlock &tip, bool include_transaction_pool) const {
  Filter filter;
  filter.transaction_id(id);
  auto found_transaction = false;
  for_each(filter, tip, include_transaction_pool,
           [&](const messages::TaggedTransaction &match) {
             if (!found_transaction) {
               tagged_transaction->CopyFrom(match);
               found_transaction = true;
             }
             return false;
           });
  return found_transaction;
}

bool LedgerMongodb::get_transaction(const messages::Hash &id,
                                    messages::Transaction *transaction) const {
  messages::BlockHeight block_height;
  return get_transaction(id, transaction, &block_height);
}

bool LedgerMongodb::get_transaction(const messages::Hash &id,
                                    messages::Transaction *transaction,
                                    messages::BlockHeight *blockheight) const {
  // TODO this will return a bad height if the transaction is in 2 blocks
  // but is the height really needed???
  std::lock_guard<std::mutex> lock(_ledger_mutex);
  auto query_transaction = bss::document{} << TRANSACTION_ID << to_bson(id)
                                           << bss::finalize;

  auto bson_transactions =
      _transactions.find(std::move(query_transaction), remove_OID());

  for (const auto &bson_transaction : bson_transactions) {
    messages::TaggedTransaction tagged_transaction;
    from_bson(bson_transaction, &tagged_transaction);
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
  auto lookup = bss::document{} << FROM << BLOCKS << FOREIGN_FIELD
                                << BLOCK_HEADER_ID << LOCAL_FIELD << BLOCK_ID
                                << AS << TAGGED_BLOCK << bss::finalize;

  auto match = bss::document{} << TAGGED_BLOCK + "." + BRANCH << MAIN
                               << bss::finalize;

  auto group = bss::document{} << _ID << bsoncxx::types::b_null{} << COUNT
                               << bss::open_document << "$sum" << 1
                               << bss::close_document << bss::finalize;

  mongocxx::pipeline pipeline;
  pipeline.lookup(lookup.view());
  pipeline.match(match.view());
  pipeline.group(group.view());
  auto cursor = _transactions.aggregate(pipeline);
  return (*cursor.begin())[COUNT].get_int32();
}

std::size_t LedgerMongodb::total_nb_blocks() const {
  std::lock_guard<std::mutex> lock(_ledger_mutex);
  auto query = bss::document{} << BRANCH << MAIN_BRANCH_NAME << bss::finalize;
  return _blocks.count(std::move(query));
}

bool LedgerMongodb::for_each(const Filter &filter,
                             const messages::TaggedBlock &tip,
                             bool include_transaction_pool,
                             Functor functor) const {
  std::lock_guard<std::mutex> lock(_ledger_mutex);
  if (!filter.output_address() && !filter.input_transaction_id() &&
      !filter.transaction_id() && !filter.output_id()) {
    LOG_WARNING << "missing filters for for_each query";
    return false;
  }

  auto query = bss::document{};

  if (filter.output_address()) {
    const auto bson = to_bson(*filter.output_address());
    query << TRANSACTION + "." + OUTPUTS + "." + ADDRESS << bson;
  }

  if (filter.transaction_id()) {
    query << TRANSACTION_ID << to_bson(*filter.transaction_id());
  }

  if (filter.input_transaction_id() && filter.output_id()) {
    query << TRANSACTION + "." + INPUTS << bss::open_document << "$elemMatch"
          << bss::open_document << ID << to_bson(*filter.input_transaction_id())
          << OUTPUT_ID << *filter.output_id() << bss::close_document
          << bss::close_document;
  } else if (filter.input_transaction_id()) {
    query << TRANSACTIONS + "." + INPUTS + "." + ID
          << to_bson(*filter.input_transaction_id());
  } else if (filter.output_id()) {
    query << TRANSACTIONS + "." + INPUTS + "." + OUTPUT_ID
          << *filter.output_id();
  }

  auto bson_transactions =
      _transactions.find((query << bss::finalize).view(), remove_OID());

  bool applied_functor = false;
  for (const auto &bson_transaction : bson_transactions) {
    messages::TaggedTransaction tagged_transaction;
    from_bson(bson_transaction, &tagged_transaction);
    if (!tagged_transaction.has_block_id()) {
      if (include_transaction_pool) {
        functor(tagged_transaction);
        applied_functor = true;
      }
      continue;
    }
    messages::TaggedBlock tagged_block;
    unsafe_get_block(tagged_transaction.block_id(), &tagged_block, false);
    if (is_ancestor(tagged_block, tip)) {
      functor(tagged_transaction);
      applied_functor = true;
    }
  }

  return applied_functor;
}

bool LedgerMongodb::for_each(const Filter &filter, Functor functor) const {
  return for_each(filter, _main_branch_tip, true, functor);
}

messages::BranchID LedgerMongodb::new_branch_id() const {
  auto query = bss::document{} << bss::finalize;
  mongocxx::options::find find_options;

  auto projection_doc = bss::document{} << _ID << 0
                                        << BRANCH_PATH + "." + BRANCH_IDS << 1
                                        << bss::finalize;

  find_options.projection(std::move(projection_doc));
  find_options.sort(bss::document{} << BRANCH_PATH + "." + BRANCH_IDS + ".0"
                                    << -1 << bss::finalize);

  auto max_branch_id = _blocks.find_one(std::move(query), find_options);
  return max_branch_id->view()[BRANCH_PATH][BRANCH_IDS][0].get_int32() + 1;
}

bool LedgerMongodb::add_transaction(
    const messages::TaggedTransaction &tagged_transaction) {
  std::lock_guard<std::mutex> lock(_ledger_mutex);
  auto bson_transaction = to_bson(tagged_transaction);
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
  tagged_transaction.set_is_coinbase(false);
  tagged_transaction.mutable_transaction()->CopyFrom(transaction);
  return add_transaction(tagged_transaction);
}

bool LedgerMongodb::delete_transaction(const messages::Hash &id) {
  // Delete a transaction in the transaction pool
  std::lock_guard<std::mutex> lock(_ledger_mutex);
  auto query = bss::document{} << TRANSACTION_ID << to_bson(id) << BLOCK_ID
                               << bsoncxx::types::b_null{} << bss::finalize;
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
  auto query = bss::document{} << BLOCK_ID << bsoncxx::types::b_null{}
                               << bss::finalize;

  auto options = projection(TRANSACTION);
  options.sort(bss::document{} << TRANSACTION_ID << 1 << bss::finalize);
  auto cursor = _transactions.find(std::move(query), options);

  int num_transactions = 0;
  for (const auto &bson_transaction : cursor) {
    num_transactions++;
    auto transaction = block->add_transactions();
    from_bson(bson_transaction[TRANSACTION].get_document(), transaction);
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
  auto filter = bss::document{} << BLOCK_HEADER_ID << to_bson(block_header.id())
                                << bss::finalize;
  auto update = bss::document{} << $SET << bss::open_document << BRANCH_PATH
                                << to_bson(branch_path) << BRANCH
                                << UNVERIFIED_BRANCH_NAME << bss::close_document
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

bool LedgerMongodb::get_unverified_blocks(
    std::vector<messages::TaggedBlock> *tagged_blocks) const {
  std::lock_guard<std::mutex> lock(_ledger_mutex);
  auto query = bss::document{} << BRANCH << UNVERIFIED_BRANCH_NAME
                               << bss::finalize;
  auto options = remove_OID();
  options.sort(bss::document{} << BLOCK_HEADER_HEIGHT << 1 << bss::finalize);
  auto result = _blocks.find(std::move(query), options);
  for (const auto &bson_block : result) {
    auto &tagged_block = tagged_blocks->emplace_back();
    from_bson(bson_block, &tagged_block);
    fill_block_transactions(tagged_block.mutable_block());
  }
  return true;
}

bool LedgerMongodb::set_block_verified(
    const messages::Hash &id, const messages::BlockScore &score,
    const messages::Hash previous_assembly_id) {
  std::lock_guard<std::mutex> lock(_ledger_mutex);
  messages::TaggedBlock previous;
  auto filter = bss::document{} << BLOCK_HEADER_ID << to_bson(id)
                                << bss::finalize;
  auto update = bss::document{}
                << $SET << bss::open_document << SCORE << score << BRANCH
                << FORK_BRANCH_NAME << PREVIOUS_ASSEMBLY_ID
                << to_bson(previous_assembly_id) << bss::close_document
                << bss::finalize;
  auto update_result = _blocks.update_one(std::move(filter), std::move(update));
  return update_result && update_result->modified_count() > 0;
}

bool LedgerMongodb::update_branch_tag(const messages::Hash &id,
                                      const messages::Branch &branch) {
  auto filter = bss::document{} << BLOCK_HEADER_ID << to_bson(id)
                                << bss::finalize;
  auto update = bss::document{} << $SET << bss::open_document << BRANCH
                                << messages::Branch_Name(branch)
                                << bss::close_document << bss::finalize;
  auto update_result = _blocks.update_one(std::move(filter), std::move(update));
  return update_result && update_result->modified_count() > 0;
}

bool LedgerMongodb::update_main_branch(messages::TaggedBlock *main_branch_tip) {
  std::lock_guard<std::mutex> lock(_ledger_mutex);
  auto query = bss::document{} << bss::finalize;
  auto options = remove_OID();
  options.sort(bss::document{} << SCORE << -1 << bss::finalize);
  auto bson_block = _blocks.find_one(std::move(query), options);
  if (!bson_block) {
    return false;
  }
  from_bson(bson_block->view(), main_branch_tip);

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

bool LedgerMongodb::get_pii(const messages::Address &address,
                            const messages::Hash &assembly_id,
                            Double *pii) const {
  std::lock_guard<std::mutex> lock(_ledger_mutex);
  auto query = bss::document{} << ADDRESS << to_bson(address) << ASSEMBLY_ID
                               << to_bson(assembly_id) << bss::finalize;

  const auto result = _blocks.find_one(std::move(query), remove_OID());
  if (!result) {
    return false;
  }

  *pii = result->view()[SCORE].get_double();
  return true;
}

bool LedgerMongodb::save_pii(const messages::Address &address,
                             const messages::Hash &assembly_id,
                             const Double &score) {
  std::lock_guard<std::mutex> lock(_ledger_mutex);
  messages::Pii pii;
  pii.mutable_address()->CopyFrom(address);
  pii.mutable_assembly_id()->CopyFrom(assembly_id);
  pii.set_score(score);
  auto bson_pii = to_bson(pii);
  auto result = _blocks.insert_one(std::move(bson_pii));
  if (!result) {
    return false;
  }
  return true;
}

void LedgerMongodb::empty_database() {
  std::lock_guard<std::mutex> lock(_ledger_mutex);
  _db.drop();
}

bool LedgerMongodb::unsafe_get_assembly(const messages::Hash &assembly_id,
                                        messages::Assembly *assembly) const {
  auto query = bss::document{} << ASSEMBLY_ID << to_bson(assembly_id)
                               << bss::finalize;
  const auto result = _assemblies.find_one(std::move(query), remove_OID());
  if (!result) {
    return false;
  }
  from_bson(result->view(), assembly);
  return true;
}

bool LedgerMongodb::get_assembly(const messages::Hash &assembly_id,
                                 messages::Assembly *assembly) const {
  std::lock_guard<std::mutex> lock(_ledger_mutex);
  return unsafe_get_assembly(assembly_id, assembly);
}

bool LedgerMongodb::get_next_assembly(const messages::Hash &assembly_id,
                                      messages::Assembly *assembly) const {
  std::lock_guard<std::mutex> lock(_ledger_mutex);
  auto query = bss::document{} << PREVIOUS_ASSEMBLY_ID << to_bson(assembly_id)
                               << bss::finalize;
  const auto result = _assemblies.find_one(std::move(query), remove_OID());
  if (!result) {
    return false;
  }
  from_bson(result->view(), assembly);
  return true;
}

bool LedgerMongodb::add_assembly(const messages::TaggedBlock &tagged_block,
                                 const messages::BlockHeight &first_height,
                                 const messages::BlockHeight &last_height) {
  std::lock_guard<std::mutex> lock(_ledger_mutex);
  if (tagged_block.branch() != messages::Branch::MAIN &&
      tagged_block.branch() != messages::Branch::FORK) {
    return false;
  }
  messages::Assembly assembly;
  if (unsafe_get_assembly(tagged_block.block().header().id(), &assembly)) {
    // The assembly already exist
    return false;
  }
  assembly.mutable_assembly_id()->CopyFrom(tagged_block.block().header().id());
  if (!tagged_block.has_previous_assembly_id()) {
    if (tagged_block.block().header().height() != 0) {
      throw(
          "Something is wrong here only the block0 should not have a "
          "previous_assembly_id");
    }
  } else {
    assembly.mutable_previous_assembly_id()->CopyFrom(
        tagged_block.previous_assembly_id());
  }
  assembly.set_finished_computation(false);
  assembly.set_first_height(first_height);
  assembly.set_last_height(first_height);
  auto bson_assembly = to_bson(assembly);
  auto result = _assemblies.insert_one(std::move(bson_assembly));
  return (bool)result;
}

bool LedgerMongodb::get_pii(const messages::Address &address,
                            const messages::Hash &assembly_id,
                            messages::Pii *pii) const {
  std::lock_guard<std::mutex> lock(_ledger_mutex);
  auto query = bss::document{} << ADDRESS << to_bson(address) << ASSEMBLY_ID
                               << to_bson(assembly_id) << bss::finalize;
  auto result = _pii.find_one(std::move(query), remove_OID());
  if (!result) {
    return false;
  };
  from_bson(result->view(), pii);
  return true;
}

bool LedgerMongodb::set_pii(const messages::Pii &pii) {
  std::lock_guard<std::mutex> lock(_ledger_mutex);
  auto bson_pii = to_bson(pii);
  auto result = _pii.insert_one(std::move(bson_pii));
  return (bool)result;
}

bool LedgerMongodb::set_previous_assembly_id(
    const messages::Hash &block_id,
    const messages::Hash &previous_assembly_id) {
  std::lock_guard<std::mutex> lock(_ledger_mutex);
  auto filter = bss::document{} << BLOCK_HEADER_ID << to_bson(block_id)
                                << bss::finalize;
  auto update = bss::document{}
                << $SET << bss::open_document << PREVIOUS_ASSEMBLY_ID
                << to_bson(previous_assembly_id) << bss::close_document
                << bss::finalize;
  auto update_result = _blocks.update_one(std::move(filter), std::move(update));
  return update_result && update_result->modified_count() > 0;
}

bool LedgerMongodb::get_assemblies_to_compute(
    std::vector<messages::Assembly> *assemblies) const {
  std::lock_guard<std::mutex> lock(_ledger_mutex);
  auto query = bss::document{} << FINISHED_COMPUTATION << false
                               << bss::finalize;
  auto bson_assemblies = _assemblies.find(std::move(query), remove_OID());

  if (bson_assemblies.begin() == bson_assemblies.end()) {
    return false;
  }

  for (const auto &bson_assembly : bson_assemblies) {
    auto &assembly = assemblies->emplace_back();
    from_bson(bson_assembly, &assembly);
  }

  return true;
}

bool LedgerMongodb::get_block_writer(const messages::Hash &assembly_id,
                                     int32_t address_rank,
                                     messages::Address *address) const {
  std::lock_guard<std::mutex> lock(_ledger_mutex);
  auto query = bss::document{} << ASSEMBLY_ID << to_bson(assembly_id) << RANK
                               << address_rank << bss::finalize;
  auto result = _pii.find_one(std::move(query), projection(ADDRESS));
  if (!result) {
    return false;
  };
  from_bson(result->view()[ADDRESS].get_document(), address);
  return true;
}

bool LedgerMongodb::set_nb_addresses(const messages::Hash &assembly_id,
                                     int32_t nb_addresses) {
  std::lock_guard<std::mutex> lock(_ledger_mutex);
  auto filter = bss::document{} << ASSEMBLY_ID << to_bson(assembly_id)
                                << bss::finalize;
  auto update = bss::document{} << $SET << bss::open_document << NB_ADDRESSES
                                << nb_addresses << bss::close_document
                                << bss::finalize;
  auto update_result =
      _assemblies.update_one(std::move(filter), std::move(update));
  return update_result && update_result->modified_count() > 0;
}

bool LedgerMongodb::set_seed(const messages::Hash &assembly_id, int32_t seed) {
  std::lock_guard<std::mutex> lock(_ledger_mutex);
  auto filter = bss::document{} << ASSEMBLY_ID << to_bson(assembly_id)
                                << bss::finalize;
  auto update = bss::document{} << $SET << bss::open_document << SEED << seed
                                << bss::close_document << bss::finalize;
  auto update_result =
      _assemblies.update_one(std::move(filter), std::move(update));
  return update_result && update_result->modified_count() > 0;
}

}  // namespace ledger
}  // namespace neuro
