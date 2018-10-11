#ifndef NEURO_SRC_LEDGERMONGODB_HPP
#define NEURO_SRC_LEDGERMONGODB_HPP

#include "ledger/Ledger.hpp"
#include "ledger/mongo.hpp"
#include "common/logger.hpp"
#include "messages.pb.h"
#include "config.pb.h"

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
    mutable mongocxx::collection _blocks_forks;

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
        auto query = bss::document{} << "blockId" << id << bss::finalize;

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


    void init_block0( messages::config::Database &db ) {
        messages::Block block0;
        if (!get_block(0,&block0)) {
            messages::Block block0file;
            std::ifstream t(db.block0_path());
            std::string str((std::istreambuf_iterator<char>(t)),
                            std::istreambuf_iterator<char>());

            auto d = bss::document{};
            switch(db.block0_format() ) {
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
        }
    }

  public:
    LedgerMongodb(const std::string &url, const std::string &db_name);
    LedgerMongodb(messages::config::Database &db);

    ~LedgerMongodb() {}

    void remove_all() {
        _blocks.delete_many(bss::document{} .view());
        _blocks_forks.delete_many(bss::document{} .view());
        _transactions.delete_many(bss::document{} .view());
    }

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
                neuro::messages::Transaction _transaction(transaction);
                _transaction.clear_id();
                _transaction.clear_block_id();

                Buffer buf;
                messages::to_buffer(_transaction,&buf);
                messages::Hasher newit(buf);
                transaction.mutable_id()->CopyFrom(newit);
            }

            if(!transaction.has_block_id()) {
                transaction.mutable_block_id()->CopyFrom(header.id());
            }

            auto bson_transaction = messages::to_bson(transaction);
            _transactions.insert_one(std::move(bson_transaction));
        }
        return true;
    }

    bool get_transaction( messages::Hash &id, messages::Transaction * transaction ) {
        auto query_transaction = bss::document{} << "id" << messages::to_bson(id) << bss::finalize;

        mongocxx::options::find findoption;
        auto projection_transaction = bss::document{};
        projection_transaction << "_id" << 0;  ///!< remove _id objectID
        findoption.projection(projection_transaction.view());

        auto res = _transactions.find_one(query_transaction.view(), findoption);
        if ( res ) {
            messages::from_bson(res->view(), transaction);
            return true;
        }
        return false;
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
        }

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



    /**
    * Functions for forks
    */

    bool fork_add_block(const messages::Block &b) {
        auto bson_block = messages::to_bson(b);
        auto res = _blocks_forks.insert_one(std::move(bson_block));
        if ( res ) {
            return true ; //(res->result().inserted_count () == 1);
        }
        return false;
    }

    bool fork_delete_block(messages::Hash &id) {
        auto query_block = bss::document{} << "id" << messages::to_bson(id) << bss::finalize;
        auto res = _blocks_forks.delete_one(query_block.view());
        if ( res ) {
            return true;
        }
        return false;
    }

    void fork_for_each(Functor_block functor) {
        auto query_block = bss::document{} << bss::finalize;

        mongocxx::options::find findoption;
        auto projection_transaction = bss::document{};
        projection_transaction << "_id" << 0;  ///!< remove _id objectID
        findoption.projection(projection_transaction.view());

        auto cursor_block = _blocks_forks.find(query_block.view(),findoption);

        for (auto &bson_block : cursor_block) {
            messages::Block block;
            messages::from_bson(bson_block, &block);
            functor(block);
        }
    }

    void fork_test() {
        auto query_block = bss::document{} << bss::finalize;
        _blocks_forks.delete_many( query_block.view() );
    }
};

}  // namespace ledger
}  // namespace neuro

#endif /* NEURO_SRC_LEDGERMONGODB_HPP */
