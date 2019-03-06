#include <gtest/gtest.h>
#include <thread>
#include "consensus/Consensus.hpp"
#include "ledger/LedgerMongodb.hpp"
#include "tooling/Simulator.hpp"

namespace neuro {
namespace tooling {
namespace tests {

using namespace std::chrono;

class RealtimeSimulator : public testing::Test {
 public:
  const std::string db_url = "mongodb://mongo:27017";
  const std::string db_name = "test_simulator";
  const messages::NCCAmount ncc_block0 = messages::NCCAmount(100000);
  const int nb_keys = 100;

 public:
  Simulator simulator;
  std::shared_ptr<neuro::ledger::LedgerMongodb> ledger;
  std::shared_ptr<consensus::Consensus> consensus;

  RealtimeSimulator()
      : simulator(tooling::Simulator::Simulator::RealtimeSimulator(
            db_url, db_name, nb_keys, ncc_block0)),
        ledger(simulator.ledger),
        consensus(simulator.consensus) {}

  void test_simulation() {
    messages::Block block0;
    ASSERT_TRUE(ledger->get_block(0, &block0));
    uint64_t block0_timestamp = block0.header().timestamp().data();
    const auto nb_transactions = 2;
    for (uint32_t i = 1; i < 3 * consensus->config().blocks_per_assembly; i++) {
      for (auto j = 0; j < nb_transactions; j++) {
        int sender_index = rand() % nb_keys;
        int recipient_index = rand() % nb_keys;

        auto t0 = Timer::now();
        auto transaction =
            ledger->send_ncc(simulator.keys[sender_index].private_key(),
                             simulator.addresses[recipient_index], 0.5);
        LOG_DEBUG << "SEND_NCC TOOK " << (Timer::now() - t0).count() / 1E6
                  << " MS";

        auto t1 = Timer::now();
        ASSERT_TRUE(consensus->add_transaction(transaction));
        LOG_DEBUG << "ADD_TRANSACTION TOOK "
                  << (Timer::now() - t1).count() / 1E6 << " MS";
      }
      milliseconds sleep_time =
          (milliseconds)(
              1000 * (block0_timestamp + i * consensus->config().block_period) +
              500 * consensus->config().block_period) -
          duration_cast<milliseconds>(system_clock::now().time_since_epoch());
      LOG_DEBUG << "WAITING " << sleep_time.count() << " MS FOR BLOCK " << i
                << std::endl;
      std::this_thread::sleep_for(sleep_time);
      messages::TaggedBlock tagged_block;

      auto t2 = Timer::now();
      ASSERT_TRUE(simulator.ledger->get_last_block(&tagged_block));
      LOG_DEBUG << "GET LAST BLOCK TOOK " << (Timer::now() - t2).count() / 1E6
                << " MS";

      // Make sure that we finished computing the pii of the previous assembly
      // because we will need it soon
      if (i % consensus->config().blocks_per_assembly >=
          consensus->config().blocks_per_assembly - 1) {
        messages::Assembly assembly;
        ASSERT_TRUE(ledger->get_assembly(tagged_block.previous_assembly_id(),
                                         &assembly));
        ASSERT_TRUE(assembly.finished_computation());
        ASSERT_GT(assembly.nb_addresses(), 1);

        std::vector<std::pair<messages::BlockHeight, consensus::AddressIndex>>
            heights;
        consensus->_heights_to_write_mutex.lock();
        ASSERT_GT(consensus->_heights_to_write.size(), 1);
        consensus->_heights_to_write_mutex.unlock();
      }

      ASSERT_EQ(tagged_block.block().header().height(), i);
      ASSERT_EQ(tagged_block.block().transactions_size(), nb_transactions);
      ASSERT_EQ(tagged_block.block().coinbases_size(), 1);
    }
  }
};

TEST_F(RealtimeSimulator, simulation) { test_simulation(); }

}  // namespace tests
}  // namespace tooling
}  // namespace neuro
