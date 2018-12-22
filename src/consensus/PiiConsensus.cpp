#include "PiiConsensus.hpp"
#include "common/logger.hpp"
#include "ledger/Ledger.hpp"
#include "messages/Message.hpp"

namespace neuro {
namespace consensus {

PiiConsensus::PiiConsensus(std::shared_ptr<boost::asio::io_context> io,
                           std::shared_ptr<ledger::Ledger> ledger,
                           std::shared_ptr<networking::Networking> network,
                           int32_t block_assembly)
    : _ledger(ledger),
      _network(network),
      _transaction_pool(_ledger),
      _ForkManager(ledger, _transaction_pool),
      // _valide_block(true),
      _timer_of_block_time(*io) {
  // load all block from
}

void PiiConsensus::init() {}

int32_t PiiConsensus::next_height_by_time() const { return 0; }

void PiiConsensus::timer_func() {}
// TO DO Test
void PiiConsensus::build_block() {}

void PiiConsensus::add_transaction(const messages::Transaction &transaction) {}

bool PiiConsensus::block_in_ledger(const messages::Hash &id) { return true; }

void PiiConsensus::ckeck_run_assembly(int32_t height) {}

void PiiConsensus::add_block(const neuro::messages::Block &block,
                             bool check_time) {}  // namespace consensus

void PiiConsensus::add_blocks(
    const std::vector<neuro::messages::Block *> &blocks) {}

void PiiConsensus::random_from_hashs() {}

uint32_t PiiConsensus::ramdon_at(int index, uint64_t nonce) const { return 0; }

Address PiiConsensus::get_next_owner() const {
  Address address;
  return address;
}

bool PiiConsensus::check_owner(
    const neuro::messages::BlockHeader &blockheader) const {
  return true;
}

std::string PiiConsensus::owner_at(int32_t index) { return ""; }

void PiiConsensus::add_wallet_keys(const std::shared_ptr<crypto::Ecc> wallet) {}

}  // namespace consensus
}  // namespace neuro
