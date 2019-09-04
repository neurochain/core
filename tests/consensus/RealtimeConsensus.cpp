#include <memory>
#include <gtest/gtest.h>
#include <thread>

#include "consensus.pb.h"
#include "consensus/Consensus.hpp"
#include "ledger/LedgerMongodb.hpp"
#include "messages.pb.h"
#include "messages/NCCAmount.hpp"
#include "tooling/Simulator.hpp"

namespace neuro {
namespace consensus {
namespace tests {

class RealtimeConsensus : public testing::Test {
 public:
  const std::string db_url = "mongodb://mongo:27017";
  const std::string db_name = "test_consensus";
  const messages::NCCAmount ncc_block0 = messages::NCCAmount(100000);
  const int nb_keys = 1;

 protected:
  tooling::Simulator simulator;
  std::shared_ptr<neuro::ledger::LedgerMongodb> ledger;
  std::shared_ptr<consensus::Consensus> consensus;

  RealtimeConsensus()
      : simulator(tooling::Simulator::Simulator::RealtimeSimulator(
            db_url, db_name, nb_keys, ncc_block0)),
        ledger(simulator.ledger),
        consensus(simulator.consensus) {}

 public:
  void test_update_heights_thread() {
    consensus->_is_stoped = true;
    std::vector<std::pair<messages::BlockHeight, KeyPubIndex>> heights;
    consensus->get_heights_to_write(simulator.key_pubs, &heights);
    ASSERT_GT(heights.size(), 0);
    messages::TaggedBlock tagged_block;
    ledger->get_last_block(&tagged_block);
    ASSERT_EQ(tagged_block.block().header().height(), 0);
    std::this_thread::sleep_for(consensus->config().update_heights_sleep * 2);
    consensus->_heights_to_write_mutex.lock();
    ASSERT_EQ(consensus->_heights_to_write.at(0).first, 1);
    ASSERT_EQ(consensus->_heights_to_write.at(0).second, 0);
    ASSERT_EQ(
        consensus->_heights_to_write.at(consensus->_heights_to_write.size() - 1)
            .first,
        consensus->config().blocks_per_assembly * 2 - 1);
    ASSERT_EQ(consensus->_heights_to_write.size(),
              2 * consensus->config().blocks_per_assembly - 1);
    consensus->_heights_to_write_mutex.unlock();
  }

  void test_miner_thread() {
    // Sleep for 1 more second because the block0 is 1 second in the future
    std::this_thread::sleep_for(
        (std::chrono::seconds)(consensus->config().blocks_per_assembly *
                                   consensus->config().block_period +
                               1));
    messages::TaggedBlock tagged_block;
    ASSERT_TRUE(ledger->get_last_block(&tagged_block, false));
    ASSERT_GT(tagged_block.block().header().height(),
              consensus->config().blocks_per_assembly - 2);
    ASSERT_LT(tagged_block.block().header().height(),
              consensus->config().blocks_per_assembly + 2);
  }

  void test_pii_thread() {
    std::this_thread::sleep_for(
        (std::chrono::seconds)(consensus->config().blocks_per_assembly + 2) *
        consensus->config().block_period);
    messages::TaggedBlock tagged_block;
    ASSERT_TRUE(ledger->get_last_block(&tagged_block, false));
    ASSERT_GT(tagged_block.block().header().height(),
              consensus->config().blocks_per_assembly - 1);
    messages::Assembly assembly;
    ASSERT_TRUE(
        ledger->get_assembly(tagged_block.previous_assembly_id(), &assembly));
    ASSERT_EQ(assembly.height(), 0);
    // ASSERT_TRUE(assembly.finished_computation());
  }
};

TEST_F(RealtimeConsensus, update_heights_thread) {
  test_update_heights_thread();
}

TEST_F(RealtimeConsensus, miner_thread) { test_miner_thread(); }

TEST_F(RealtimeConsensus, pii_thread) { test_pii_thread(); }

}  // namespace tests
}  // namespace consensus
}  // namespace neuro
