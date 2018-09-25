#ifndef NEURO_SRC_LEDGER_MONGO_HPP
#define NEURO_SRC_LEDGER_MONGO_HPP

#include <bsoncxx/builder/stream/array.hpp>
#include <bsoncxx/builder/stream/document.hpp>
#include <bsoncxx/document/value.hpp>
#include <mongocxx/client.hpp>
#include <mongocxx/instance.hpp>
#include <mongocxx/options/find.hpp>
#include <mongocxx/stdx.hpp>
#include <mongocxx/uri.hpp>

namespace neuro {
namespace ledger {

namespace bss = bsoncxx::builder::stream;
using bss::array;
using bss::close_array;
using bss::close_document;
using bss::concatenate;
using bss::document;
using bss::finalize;
using bss::open_array;
using bss::open_document;

}  // namespace ledger
}  // namespace neuro

#endif /* NEURO_SRC_LEDGER_MONGO_HPP */
