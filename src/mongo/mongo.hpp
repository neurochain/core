#ifndef NEURO_SRC_LEDGER_MONGO_HPP
#define NEURO_SRC_LEDGER_MONGO_HPP

#include <mongocxx/client.hpp>
#include <mongocxx/stdx.hpp>
#include <mongocxx/uri.hpp>
#include <mongocxx/options/find.hpp>
#include <mongocxx/instance.hpp>
#include <bsoncxx/builder/stream/document.hpp>
#include <bsoncxx/builder/stream/array.hpp>
#include <bsoncxx/json.hpp>

namespace neuro {
namespace ledger {

namespace bss = bsoncxx::builder::stream;
using bss::document;
using bss::array;
using bss::open_document;
using bss::close_document;
using bss::open_array;
using bss::close_array;
using bss::concatenate;
using bss::finalize;

}  // ledger
}  // neuro

#endif /* NEURO_SRC_LEDGER_MONGO_HPP */
