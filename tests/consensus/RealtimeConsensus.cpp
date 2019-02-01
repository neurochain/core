#include <gtest/gtest.h>
#include <thread>
#include "consensus/Consensus.hpp"
#include "ledger/LedgerMongodb.hpp"
#include "tooling/RealtimeSimulator.hpp"

namespace neuro {
namespace consensus {
namespace tests {

class RealtimeConsensus : public testing::Test {
 public:
  const std::string db_url = "mongodb://mongo:27017";
  const std::string db_name = "test_consensus";
  const messages::NCCAmount ncc_block0 = messages::NCCAmount(100000);
  const int nb_keys = 2;

 protected:
  tooling::RealtimeSimulator simulator;
  std::shared_ptr<neuro::ledger::LedgerMongodb> ledger;
  std::shared_ptr<consensus::Consensus> consensus;

  RealtimeConsensus()
      : simulator(db_url, db_name, ncc_block0),
        ledger(simulator.ledger),
        consensus(simulator.consensus) {}

 public:
  void test_update_heights_thread() {
    consensus->_stop_miner = true;
    consensus->_stop_compute_pii = true;
    std::vector<messages::BlockHeight> heights;
    consensus->get_heights_to_write(
        messages::Address(simulator.keys[0].public_key()), &heights);
    ASSERT_GT(heights.size(), 0);
    messages::TaggedBlock tagged_block;
    ledger->get_last_block(&tagged_block);
    ASSERT_EQ(tagged_block.block().header().height(), 0);
    std::this_thread::sleep_for(consensus->config.update_heights_sleep * 2);
    ASSERT_GT(consensus->_heights_to_write.size(),
              consensus->config.blocks_per_assembly);
  }

  void test_miner_thread() {}
};

TEST_F(RealtimeConsensus, update_heights_thread) {
  test_update_heights_thread();
}

TEST_F(RealtimeConsensus, miner_thread) { test_miner_thread(); }

}  // namespace tests
}  // namespace consensus
}  // namespace neuro
