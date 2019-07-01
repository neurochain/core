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
  RealtimeSimulator(){};

  void test_simulation(bool empty_assemblies = false) {
    int time_delta;
    int nb_empty_assemblies;
    if (empty_assemblies) {
      // Lets put it in the past so that there are some empty assemblies
      nb_empty_assemblies = 5;
      time_delta = -nb_empty_assemblies * 5 + 4;
    } else {
      // Lets put it in the future sa that we have time to prepare ourselves for
      // block1
      nb_empty_assemblies = 0;
      time_delta = 2;
    }

    bool start_threads = time_delta >= 0;
    Simulator simulator(db_url, db_name, nb_keys, ncc_block0, time_delta,
                        start_threads);
    std::shared_ptr<neuro::ledger::LedgerMongodb> ledger(simulator.ledger);
    std::shared_ptr<consensus::Consensus> consensus(simulator.consensus);

    if (empty_assemblies) {
      // When the timedelta is in the past the threads are not started because
      // it is assumed we are not trying to do a realtime simulation though in
      // our case we are.
      simulator.consensus->start_compute_pii_thread();
      simulator.consensus->start_update_heights_thread();
    }

    messages::Block block0;
    const auto &config = consensus->config();
    ASSERT_TRUE(ledger->get_block(0, &block0));
    uint64_t block0_timestamp = block0.header().timestamp().data();
    uint64_t begin_timestamp = block0_timestamp;
    bool started_miner = true;
    if (empty_assemblies) {
      // Sleep for n assemblies
      started_miner = false;
      begin_timestamp += nb_empty_assemblies * config.blocks_per_assembly *
                         config.block_period;
    }
    const auto nb_transactions = 2;
    for (uint32_t i = 1; i < 3 * consensus->config().blocks_per_assembly; i++) {
      for (auto j = 0; j < nb_transactions; j++) {
        int sender_index = rand() % nb_keys;
        int recipient_index = rand() % nb_keys;

        auto t0 = Timer::now();
        auto transaction =
            ledger->send_ncc(simulator.keys[sender_index].key_priv(),
                             simulator.addresses[recipient_index], 0.5);
        LOG_DEBUG << "SEND_NCC TOOK " << (Timer::now() - t0).count() / 1E6
                  << " MS";

        auto t1 = Timer::now();
        ASSERT_TRUE(consensus->add_transaction(transaction));
        LOG_DEBUG << "ADD_TRANSACTION TOOK "
                  << (Timer::now() - t1).count() / 1E6 << " MS";
      }

      if (!started_miner) {
        milliseconds sleep_time =
            (milliseconds)(1000 * (begin_timestamp + config.block_period) +
                           100) -
            duration_cast<milliseconds>(system_clock::now().time_since_epoch());
        LOG_DEBUG << "WAITING " << sleep_time.count()
                  << " MS BEFORE STARTING MINER THREAD " << std::endl;
        std::this_thread::sleep_for(sleep_time);

        started_miner = true;
        consensus->start_miner_thread();
      }

      milliseconds sleep_time =
          (milliseconds)(1000 * (begin_timestamp + i * config.block_period) +
                         500 * config.block_period) -
          duration_cast<milliseconds>(system_clock::now().time_since_epoch());
      LOG_DEBUG << "WAITING " << sleep_time.count() << " MS FOR BLOCK "
                << i + config.blocks_per_assembly * nb_empty_assemblies
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

      if (empty_assemblies) {
        ASSERT_EQ(tagged_block.block().header().height(),
                  i + nb_empty_assemblies * config.blocks_per_assembly);
      } else {
        ASSERT_EQ(tagged_block.block().header().height(), i);
      }

      ASSERT_EQ(tagged_block.block().transactions_size(), nb_transactions);
      ASSERT_EQ(tagged_block.block().coinbase().outputs_size(), 1);
    }
  }
};

TEST_F(RealtimeSimulator, simulation) { test_simulation(); }

TEST_F(RealtimeSimulator, empty_assemblies) {
  bool empty_assemblies = true;
  test_simulation(empty_assemblies);
}

}  // namespace tests
}  // namespace tooling
}  // namespace neuro
