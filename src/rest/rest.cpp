#include "rest/Rest.hpp"
#include "ledger/LedgerMongodb.hpp"

int main(int argc, char **argv) {
  auto ledger = std::make_shared<neuro::ledger::LedgerMongodb>(
      "mongodb://127.0.0.1:27017/neuro", "neuro");
  auto rest = neuro::rest::Rest(8080, ledger);
  rest.join();
  return 0;
}
