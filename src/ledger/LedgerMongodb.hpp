#ifndef NEURO_SRC_LEDGERMONGODB_HPP
#define NEURO_SRC_LEDGERMONGODB_HPP

#include "common/logger.hpp"
#include "config.pb.h"
#include "ledger/Ledger.hpp"
#include "ledger/mongo.hpp"
#include "messages.pb.h"

namespace neuro {
namespace ledger {

class LedgerMongodb : public Ledger {
 private:
  mutable mongocxx::instance _instance{};
  mutable mongocxx::uri _uri;
  mutable mongocxx::client _client;
  mutable mongocxx::database _db;
  mutable mongocxx::collection _blocks;
  mutable mongocxx::collection _transactions;

  void remove_OID(mongocxx::options::find &filter) {}

  bool get_block_header(const bsoncxx::document::view &block,
                        messages::BlockHeader *header) {
    messages::from_bson(block, header);
    return true;
  }

  // bool
  // get_block(const bsoncxx::document::view &bson_block,
  //           messages::Block *block);

  bool get_transactions_from_block(const bsoncxx::document::view &id,
                                   messages::Block *block) {
    auto query = bss::document{} << "id" << id << bss::finalize;

    mongocxx::options::find findoption;
    auto projection_transaction = bss::document{};
    projection_transaction << "_id" << 0;  ///!< remove _id objectID
    findoption.projection(projection_transaction.view());

    auto cursor = _transactions.find(std::move(query), findoption);

    for (auto &bson_transaction : cursor) {
      auto transaction = block->add_transactions();
      from_bson(bson_transaction, transaction);
    }

    return true;
  }

  bool get_transactions_from_block(const messages::BlockID &id,
                                   messages::Block *block) {
    const auto bson_id = to_bson(id);
    return get_transactions_from_block(bson_id.view(), block);
  }

  void init_block0(messages::config::Database &db) {
    messages::Block block0;
    if (!get_block(0, &block0)) {
      messages::Block block0file;
      std::ifstream t("block.0.bp");
      std::string str((std::istreambuf_iterator<char>(t)),
                      std::istreambuf_iterator<char>());
      auto d = bss::document{};
      switch (db.block0_format()) {
        case messages::config::Database::Block0Format::
            Database_Block0Format_PROTO:
          block0file.ParseFromString(str);
          break;
        case messages::config::Database::Block0Format::
            Database_Block0Format_BSON:
          d << str;
          messages::from_bson(d.view(), &block0file);
          break;
        case messages::config::Database::Block0Format::
            Database_Block0Format_JSON:
          messages::from_json(str, &block0file);
          break;
      }

      push_block(block0file);
    }
  }

 public:
  LedgerMongodb(const std::string &url, const std::string &db_name);
  LedgerMongodb(messages::config::Database &db);

  ~LedgerMongodb() {}

  messages::BlockHeight height() const {
    auto query = bss::document{} << bss::finalize;
    auto options = mongocxx::options::find{};
    options.sort(bss::document{} << "height" << -1 << bss::finalize);

    auto projection_transaction = bss::document{};
    projection_transaction << "_id" << 0;  ///!< remove _id objectID
    options.projection(projection_transaction.view());

    const auto res = _blocks.find_one(std::move(query), options);
    if (!res) {
      return 0;
    }

    return res->view()["height"].get_int32().value;
  }

  bool get_block_header(const messages::BlockID &id,
                        messages::BlockHeader *header) {
    const auto bson_id = to_bson(id);

    auto query = bss::document{} << "id" << bson_id << bss::finalize;

    mongocxx::options::find findoption;
    auto projection_transaction = bss::document{};
    projection_transaction << "_id" << 0;  ///!< remove _id objectID
    findoption.projection(projection_transaction.view());

    auto res = _blocks.find_one(std::move(query), findoption);
    if (!res) {
      return false;
    }
    return get_block_header(res->view(), header);
  }

  bool get_last_block_header(messages::BlockHeader *block_header) {
    auto query = bss::document{} << bss::finalize;
    auto options = mongocxx::options::find{};
    options.sort(bss::document{} << "height" << -1 << bss::finalize);

    auto projection_transaction = bss::document{};
    projection_transaction << "_id" << 0;  ///!< remove _id objectID
    options.projection(projection_transaction.view());

    const auto res = _blocks.find_one(std::move(query), options);
    if (!res) {
      return false;
    }

    messages::from_bson(*res, block_header);
    return true;
  }

  bool get_block(const messages::BlockID &id, messages::Block *block) {
    auto header = block->mutable_header();
    auto res_state = get_block_header(id, header);

    res_state &= get_transactions_from_block(id, block);

    return res_state;
  }

  bool get_block(const messages::BlockHeight height, messages::Block *block) {
    auto query = bss::document{} << "height" << height << bss::finalize;

    mongocxx::options::find findoption;
    auto projection_transaction = bss::document{};
    projection_transaction << "_id" << 0;  ///!< remove _id objectID
    findoption.projection(projection_transaction.view());

    const auto res = _blocks.find_one(std::move(query), findoption);
    if (!res) {
      return false;
    }

    std::cout << bsoncxx::to_json(res->view()) << std::endl;

    auto header = block->mutable_header();
    get_block_header(res->view(), header);

    get_transactions_from_block(res->view()["id"].get_document(), block);

    return true;
  }

  bool push_block(const messages::Block &block) {
    const auto header = block.header();
    auto bson_header = messages::to_bson(header);

    _blocks.insert_one(std::move(bson_header));

    for (neuro::messages::Transaction transaction : block.transactions()) {
      if (!transaction.has_id()) {
        transaction.mutable_id()->CopyFrom(header.id());
      }

      auto bson_transaction = messages::to_bson(transaction);
      _transactions.insert_one(std::move(bson_transaction));
    }
    return true;
  }

  bool for_each(const Filter &filter, Functor functor) {
    if (!filter.output()) {
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
      std::cout << "adding output filter to query transaction" << std::endl;
    }

    query_transaction << bss::finalize;
    std::cout << "query_transaction " << bsoncxx::to_json(query_transaction) << std::endl;

    // auto cursor_block = _blocks.find(query_block.view());

    mongocxx::options::find findoption;
    auto projection_transaction = bss::document{};
    projection_transaction << "_id" << 0;  ///!< remove _id objectID
    findoption.projection(projection_transaction.view());

    auto cursor_transaction =
      _transactions.find(query_transaction.view(), findoption);

    for (auto &bson_transaction : cursor_transaction) {
      messages::Transaction transaction;
      messages::from_bson(bson_transaction, &transaction);
      functor(transaction);
    }

    return true;
  }
};

}  // namespace ledger
}  // namespace neuro

#endif /* NEURO_SRC_LEDGERMONGODB_HPP */
