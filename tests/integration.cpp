#include <gtest/gtest.h>
#include "ledger/LedgerMongodb.hpp"

namespace neuro {
namespace tests {


TEST(foo, bar) {
  ASSERT_TRUE(true);
}

TEST(bar, foo) {
  const std::string config =
      "{"
      "  \"url\": \"mongodb://mongo:27017\","
      "  \"dbName\": \"test_ledger\","
      "  \"block0Format\": \"PROTO\","
      "  \"block0Path\": \"./data.0.testnet\""
      "}";
  try {
    auto ledger = std::make_shared<::neuro::ledger::LedgerMongodb>(config);
  }catch(...) {
    std::cout << "Could not launch mongodb" 
  }
}

}  // namespace tests
}  // namespace neuro
