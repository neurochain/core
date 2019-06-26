#include "consensus/Consensus.hpp"
#include <gtest/gtest.h>
#include <thread>
#include "ledger/LedgerMongodb.hpp"
#include "tooling/Simulator.hpp"

namespace neuro {
namespace consensus {
namespace tests {

class Consensus : public testing::Test {
 public:
  const std::string db_url = "mongodb://mongo:27017";
  const std::string db_name = "test_consensus";
  const messages::NCCAmount ncc_block0 = messages::NCCAmount(100000);
  const int nb_keys = 4;

 protected:
  tooling::Simulator simulator;
  std::shared_ptr<neuro::ledger::LedgerMongodb> ledger;
  std::shared_ptr<consensus::Consensus> consensus;

  Consensus()
      : simulator(tooling::Simulator::Simulator::StaticSimulator(
            db_url, db_name, nb_keys, ncc_block0)),
        ledger(simulator.ledger),
        consensus(simulator.consensus) {}

 public:
  void check_assembly_pii(const messages::Assembly &assembly) {
    ASSERT_TRUE(assembly.has_seed());
    ASSERT_TRUE(assembly.has_nb_addresses());
    ASSERT_TRUE(assembly.nb_addresses() >= 1);
    ASSERT_TRUE(assembly.finished_computation());
    for (int i = 0; i < assembly.nb_addresses(); i++) {
      messages::Address address;
      ASSERT_TRUE(ledger->get_block_writer(assembly.id(), i, &address));
    }
    std::vector<messages::Pii> piis;
    ledger->get_assembly_piis(assembly.id(), &piis);
    ASSERT_EQ(piis.size(), assembly.nb_addresses());
    for (const auto pii : piis) {
      ASSERT_GT(Double(pii.score()), 1);
    }
  }

  void test_is_valid_transaction() {
    // Create 3 blocks with 5 transactions each
    simulator.run(3, 5);
    const auto tip = ledger->get_main_branch_tip();
    ASSERT_EQ(tip.block().header().height(), 3);

    for (int i = 0; i <= 3; i++) {
      messages::Block block;
      ledger->get_block(i, &block);
      for (const auto &transaction : block.transactions()) {
        messages::TaggedTransaction tagged_transaction;
        tagged_transaction.mutable_transaction()->CopyFrom(transaction);
        tagged_transaction.mutable_block_id()->CopyFrom(block.header().id());
        tagged_transaction.set_is_coinbase(false);
        ASSERT_TRUE(consensus->check_id(tagged_transaction));
        ASSERT_TRUE(consensus->check_signatures(tagged_transaction));
        ASSERT_TRUE(consensus->check_inputs(tagged_transaction));
        ASSERT_TRUE(consensus->check_double_inputs(tagged_transaction));
        ASSERT_TRUE(consensus->check_outputs(tagged_transaction));
        ASSERT_TRUE(consensus->is_valid(tagged_transaction));
      }
      messages::TaggedTransaction tagged_coinbase;
      tagged_coinbase.mutable_transaction()->CopyFrom(block.coinbase());
      tagged_coinbase.mutable_block_id()->CopyFrom(block.header().id());
      tagged_coinbase.set_is_coinbase(true);
      ASSERT_TRUE(consensus->check_id(tagged_coinbase));
      ASSERT_TRUE(consensus->check_coinbase(tagged_coinbase));
      ASSERT_TRUE(consensus->is_valid(tagged_coinbase));
    }
  }

  void test_is_valid_block() {
    // Create 3 blocks with 5 transactions each
    simulator.run(3, 5);
    messages::TaggedBlock tagged_block;
    for (int i = 0; i <= 3; i++) {
      ASSERT_TRUE(ledger->get_block(i, &tagged_block));
      ASSERT_TRUE(consensus->check_transactions_order(tagged_block));
      ASSERT_TRUE(consensus->check_block_id(tagged_block));
      ASSERT_TRUE(consensus->check_block_size(tagged_block));
      ASSERT_TRUE(consensus->check_block_timestamp(tagged_block));
      if (i == 0) {
        ASSERT_FALSE(consensus->is_valid(tagged_block));
      } else {
        ASSERT_TRUE(consensus->check_block_transactions(tagged_block));
        ASSERT_TRUE(consensus->check_block_height(tagged_block));
        ASSERT_TRUE(consensus->check_block_author(tagged_block));
        ASSERT_TRUE(consensus->is_valid(tagged_block));
      }
    }
  }

  void test_compute_assembly_pii() {
    std::vector<messages::Assembly> assemblies;
    bool compute_pii = false;
    simulator.run(consensus->config().blocks_per_assembly, 10, compute_pii);
    ASSERT_TRUE(ledger->get_assemblies_to_compute(&assemblies));
    ASSERT_EQ(assemblies.size(), 1);
    auto &assembly = assemblies[0];
    ASSERT_TRUE(consensus->compute_assembly_pii(assembly));
    ASSERT_TRUE(ledger->get_assembly(assembly.id(), &assembly));
    check_assembly_pii(assembly);
  }

  void test_start_computations() {
    std::vector<messages::Assembly> assemblies;
    ASSERT_FALSE(ledger->get_assemblies_to_compute(&assemblies));
    bool compute_pii = false;
    simulator.run(consensus->config().blocks_per_assembly, 10, compute_pii);
    ASSERT_TRUE(ledger->get_assemblies_to_compute(&assemblies));
    ASSERT_EQ(assemblies.size(), 1);
    auto assembly_id = assemblies[0].id();

    // Make sure that the compute_assembly thread has started the computation
    consensus->start_compute_pii_thread();
    std::this_thread::sleep_for(consensus->config().compute_pii_sleep);

    // Wait for the computation to be finished
    consensus->_stop_compute_pii = true;
    if (consensus->_compute_pii_thread.joinable()) {
      consensus->_compute_pii_thread.join();
    }
    ASSERT_FALSE(ledger->get_assemblies_to_compute(&assemblies));
    messages::Assembly assembly;
    ASSERT_TRUE(ledger->get_assembly(assembly_id, &assembly));
    ASSERT_TRUE(assembly.has_seed());
    ASSERT_TRUE(assembly.has_nb_addresses());
    ASSERT_TRUE(assembly.nb_addresses() >= 1);
    for (int i = 0; i < assembly.nb_addresses(); i++) {
      messages::Address address;
      ASSERT_TRUE(ledger->get_block_writer(assembly_id, i, &address));
    }
  }

  void test_build_block() {
    messages::TaggedBlock tagged_block;
    messages::Assembly assembly_minus_1, assembly_minus_2;
    bool include_transactions = false;
    ASSERT_TRUE(ledger->get_last_block(&tagged_block, include_transactions));
    ledger->get_assembly(tagged_block.previous_assembly_id(),
                         &assembly_minus_1);
    ledger->get_assembly(assembly_minus_1.previous_assembly_id(),
                         &assembly_minus_2);
    for (uint32_t i = 1; i < consensus->config().blocks_per_assembly; i++) {
      messages::Address address;
      ASSERT_TRUE(consensus->get_block_writer(assembly_minus_2, i, &address));
      int address_index = -1;
      for (size_t i = 0; i < simulator.addresses.size(); i++) {
        if (address == simulator.addresses[i]) {
          address_index = i;
          break;
        }
      }
      ASSERT_NE(address_index, -1);

      const auto &keys = simulator.keys[address_index];
      messages::Block block;
      ASSERT_TRUE(consensus->build_block(keys, i, &block));
      ASSERT_EQ(block.header().height(), i);
      ASSERT_EQ(block.coinbase().outputs_size(), 1);
      ASSERT_EQ(block.header().has_id(), 1);
      ASSERT_EQ(block.header().has_author(), 1);
      messages::TaggedBlock tagged_block;
      tagged_block.mutable_block()->CopyFrom(block);

      // The timestamp of the block should be wrong we don't write it at the
      // correct time
      ASSERT_FALSE(consensus->check_block_timestamp(tagged_block));

      // So we fix it
      ASSERT_TRUE(ledger->get_block(block.header().previous_block_hash(),
                                    &tagged_block));
      block.mutable_header()->mutable_timestamp()->set_data(
          tagged_block.block().header().timestamp().data() +
          consensus->config().block_period);

      // Rewrite the block hash which should have changed
      messages::set_block_hash(&block);

      // Since we changed the hash we also need to change the signature
      crypto::sign(keys, &block);

      ASSERT_TRUE(consensus->add_block(block));
      std::vector<messages::TaggedBlock> blocks;
      ledger->get_blocks_by_previd(block.header().previous_block_hash(),
                                   &blocks);
      ASSERT_EQ(blocks.size(), 1);

      // Try inserting the same block again
      ASSERT_FALSE(consensus->add_block(block));
      blocks.clear();
      ledger->get_blocks_by_previd(block.header().previous_block_hash(),
                                   &blocks);
      ASSERT_EQ(blocks.size(), 1);
    }
  }

  uint64_t total_money() {
    uint64_t total = 0;
    for (const auto &address : simulator.addresses) {
      total += ledger->balance(address).value();
    }
    return total;
  }

  void test_money_supply() {
    ASSERT_EQ(total_money(), nb_keys * ncc_block0.value());
    for (int i = 0; i < 10; i++) {
      simulator.run(1, 5);
      ASSERT_EQ(
          total_money(),
          nb_keys * ncc_block0.value() +
              (i + 1) * simulator.consensus->config().block_reward.value());
    }
  }

  void test_send_ncc() {
    bool did_throw = false;
    try {
      auto transaction = ledger->send_ncc(simulator.keys[0].key_priv(),
                                          simulator.addresses[1], -2);
    } catch (std::runtime_error) {
      did_throw = true;
    }
    ASSERT_TRUE(did_throw);
    did_throw = false;
    try {
      auto transaction = ledger->send_ncc(simulator.keys[0].key_priv(),
                                          simulator.addresses[1], 2);
    } catch (std::runtime_error) {
      did_throw = true;
    }
    ASSERT_TRUE(did_throw);

    auto fees = messages::NCCAmount(100);
    auto transaction = ledger->send_ncc(simulator.keys[0].key_priv(),
                                        simulator.addresses[1], 0.5, fees);
    ASSERT_TRUE(consensus->add_transaction(transaction));

    transaction = ledger->send_ncc(simulator.keys[0].key_priv(),
                                   simulator.addresses[1], 0, fees);
    ASSERT_TRUE(consensus->add_transaction(transaction));

    transaction = ledger->send_ncc(simulator.keys[0].key_priv(),
                                   simulator.addresses[1], 1, fees);
    ASSERT_TRUE(consensus->add_transaction(transaction));

    // The fee is higher than the inputs
    did_throw = false;
    try {
      transaction = ledger->send_ncc(simulator.keys[0].key_priv(),
                                     simulator.addresses[1], 1, fees);
    } catch (std::runtime_error) {
      did_throw = true;
    }
    ASSERT_TRUE(did_throw);
  }

  messages::TaggedTransaction new_tagged_transaction() {
    auto fees = messages::NCCAmount(100);
    auto transaction = ledger->send_ncc(simulator.keys[0].key_priv(),
                                        simulator.addresses[1], 0.5, fees);
    messages::TaggedTransaction tagged_transaction;
    tagged_transaction.set_is_coinbase(false);
    tagged_transaction.mutable_transaction()->CopyFrom(transaction);
    return tagged_transaction;
  }

  void test_check_transaction_id() {
    auto tagged_transaction = new_tagged_transaction();
    ASSERT_TRUE(consensus->check_id(tagged_transaction));
    ASSERT_TRUE(consensus->is_valid(tagged_transaction));
    auto data =
        tagged_transaction.mutable_transaction()->mutable_id()->mutable_data();
    (*data)[0] += 1;
    ASSERT_FALSE(consensus->check_id(tagged_transaction));
    ASSERT_FALSE(consensus->is_valid(tagged_transaction));
  }

  void test_check_transaction_signatures() {
    auto tagged_transaction = new_tagged_transaction();
    ASSERT_TRUE(consensus->check_signatures(tagged_transaction));
    ASSERT_TRUE(consensus->is_valid(tagged_transaction));
    ASSERT_EQ(tagged_transaction.transaction().signatures_size(), 1);
    auto signature =
        tagged_transaction.mutable_transaction()->mutable_signatures(0);
    ASSERT_TRUE(signature->has_key_pub());
    ASSERT_TRUE(signature->has_signature());
    auto data = signature->mutable_signature()->mutable_data();
    (*data)[0] += 1;
    messages::set_transaction_hash(tagged_transaction.mutable_transaction());
    ASSERT_TRUE(consensus->check_id(tagged_transaction));
    ASSERT_FALSE(consensus->check_signatures(tagged_transaction));
    ASSERT_FALSE(consensus->is_valid(tagged_transaction));
  }

  void test_check_transaction_outputs() {
    auto tagged_transaction = new_tagged_transaction();
    ASSERT_TRUE(consensus->check_outputs(tagged_transaction));
    ASSERT_TRUE(consensus->is_valid(tagged_transaction));
    auto tagged_transaction_orig = tagged_transaction;

    // Empty inputs
    tagged_transaction.mutable_transaction()->mutable_inputs()->Clear();
    messages::set_transaction_hash(tagged_transaction.mutable_transaction());
    ASSERT_TRUE(consensus->check_id(tagged_transaction));
    ASSERT_FALSE(consensus->check_outputs(tagged_transaction));
    ASSERT_FALSE(consensus->is_valid(tagged_transaction));

    // Fake transaction id
    tagged_transaction = tagged_transaction_orig;
    ASSERT_EQ(tagged_transaction.transaction().inputs_size(), 1);
    auto input = tagged_transaction.mutable_transaction()->mutable_inputs(0);
    auto data = input->mutable_id()->mutable_data();
    (*data)[0] += 1;
    messages::set_transaction_hash(tagged_transaction.mutable_transaction());
    ASSERT_TRUE(consensus->check_id(tagged_transaction));
    ASSERT_FALSE(consensus->check_outputs(tagged_transaction));
    ASSERT_FALSE(consensus->is_valid(tagged_transaction));

    // Fake output_id
    tagged_transaction = tagged_transaction_orig;
    input = tagged_transaction.mutable_transaction()->mutable_inputs(0);
    input->set_output_id(17);
    messages::set_transaction_hash(tagged_transaction.mutable_transaction());
    ASSERT_TRUE(consensus->check_id(tagged_transaction));
    ASSERT_FALSE(consensus->check_outputs(tagged_transaction));
    ASSERT_FALSE(consensus->is_valid(tagged_transaction));
  }

  void test_check_transaction_inputs() {
    auto first_transaction = new_tagged_transaction();
    consensus->add_transaction(first_transaction.transaction());
    auto tagged_transaction = new_tagged_transaction();
    ASSERT_TRUE(consensus->check_inputs(tagged_transaction));
    ASSERT_TRUE(consensus->is_valid(tagged_transaction));
    ASSERT_EQ(tagged_transaction.transaction().inputs_size(), 1);
    ASSERT_EQ(first_transaction.transaction().inputs_size(), 1);
    tagged_transaction.mutable_transaction()->mutable_inputs()->CopyFrom(
        first_transaction.transaction().inputs());
    messages::set_transaction_hash(tagged_transaction.mutable_transaction());
    ASSERT_TRUE(consensus->check_id(tagged_transaction));
    ASSERT_FALSE(consensus->check_inputs(tagged_transaction));
    ASSERT_FALSE(consensus->is_valid(tagged_transaction));
  }

  void test_check_transaction_double_inputs() {
    auto tagged_transaction = new_tagged_transaction();
    ASSERT_TRUE(consensus->check_double_inputs(tagged_transaction));
    ASSERT_TRUE(consensus->is_valid(tagged_transaction));
    ASSERT_EQ(tagged_transaction.transaction().inputs_size(), 1);
    auto input = tagged_transaction.transaction().inputs(0);
    tagged_transaction.mutable_transaction()->add_inputs()->CopyFrom(input);
    messages::set_transaction_hash(tagged_transaction.mutable_transaction());
    ASSERT_TRUE(consensus->check_id(tagged_transaction));
    ASSERT_FALSE(consensus->check_double_inputs(tagged_transaction));
    ASSERT_FALSE(consensus->is_valid(tagged_transaction));
  }

  void test_check_coinbase() {
    messages::TaggedBlock last_block;
    ASSERT_TRUE(ledger->get_last_block(&last_block));

    // We need to insert the block to verify the coinbase
    auto block = simulator.new_block(0, last_block);
    ledger->insert_block(block);
    ASSERT_TRUE(ledger->get_block(block.header().id(), &last_block));
    auto coinbase = last_block.block().coinbase();
    messages::TaggedTransaction tagged_coinbase;
    tagged_coinbase.mutable_block_id()->CopyFrom(
        last_block.block().header().id());
    tagged_coinbase.mutable_transaction()->CopyFrom(coinbase);
    tagged_coinbase.set_is_coinbase(true);
    ASSERT_TRUE(consensus->check_coinbase(tagged_coinbase));
    return;

    // Reward too big
    block = simulator.new_block(0, last_block);
    ledger->insert_block(block);
    ASSERT_TRUE(ledger->get_block(block.header().id(), &last_block));
    coinbase = last_block.block().coinbase();
    auto output = coinbase.mutable_outputs(0);
    output->mutable_value()->set_value(output->value().value() + 1);
    messages::set_transaction_hash(tagged_coinbase.mutable_transaction());
    ASSERT_FALSE(consensus->check_coinbase(tagged_coinbase));
  }

  void test_check_transactions_order() {
    messages::TaggedBlock last_block, tagged_block;
    ASSERT_TRUE(ledger->get_last_block(&last_block));
    auto block = simulator.new_block(2, last_block);
    ASSERT_TRUE(consensus->check_transactions_order(block));

    block = simulator.new_block(2, last_block);
    auto transaction0 = block.transactions(0);
    auto transaction1 = block.transactions(1);
    block.mutable_transactions(0)->CopyFrom(transaction1);
    block.mutable_transactions(1)->CopyFrom(transaction0);

    // Get the author keys
    int address_index = -1;
    for (size_t i = 0; i < simulator.addresses.size(); i++) {
      if (block.coinbase().outputs(0).address() == simulator.addresses[i]) {
        address_index = i;
        break;
      }
    }
    ASSERT_NE(address_index, -1);
    const auto &keys = simulator.keys[address_index];

    // Rehash and resign
    messages::set_block_hash(&block);
    crypto::sign(keys, &block);
    ASSERT_FALSE(consensus->check_transactions_order(block));
  }

  void test_check_block_id() {
    messages::TaggedBlock last_block, tagged_block;
    ASSERT_TRUE(ledger->get_last_block(&last_block));
    auto block = simulator.new_block(2, last_block);
    tagged_block.mutable_block()->CopyFrom(block);
    ASSERT_TRUE(consensus->check_block_id(tagged_block));

    // Change the id
    auto data = tagged_block.mutable_block()
                    ->mutable_header()
                    ->mutable_id()
                    ->mutable_data();
    (*data)[0] += 1;
    ASSERT_FALSE(consensus->check_block_id(tagged_block));

    // Change something in a transaction
    tagged_block.mutable_block()->CopyFrom(block);
    tagged_block.mutable_block()
        ->mutable_transactions(0)
        ->mutable_outputs(0)
        ->set_data("toto");
    ASSERT_FALSE(consensus->check_block_id(tagged_block));

    // Change something in the coinbase
    tagged_block.mutable_block()->CopyFrom(block);
    tagged_block.mutable_block()
        ->mutable_coinbase()
        ->mutable_outputs(0)
        ->set_data("toto");
    ASSERT_FALSE(consensus->check_block_id(tagged_block));

    // Change something in the header
    tagged_block.mutable_block()->CopyFrom(block);
    data = tagged_block.mutable_block()
               ->mutable_header()
               ->mutable_previous_block_hash()
               ->mutable_data();
    (*data)[0] += 1;
    ASSERT_FALSE(consensus->check_block_id(tagged_block));

    // Change the author but this should not have an impact because the author
    // is set after the hash to facilitate denunciations
    tagged_block.mutable_block()->CopyFrom(block);
    tagged_block.mutable_block()->mutable_header()->mutable_author()->Clear();
    ASSERT_TRUE(consensus->check_block_id(tagged_block));
  }

  void test_check_block_size() {
    messages::TaggedBlock last_block, tagged_block;
    ASSERT_TRUE(ledger->get_last_block(&last_block));
    auto block = simulator.new_block(2, last_block);
    tagged_block.mutable_block()->CopyFrom(block);
    ASSERT_TRUE(consensus->check_block_size(tagged_block));

    std::stringstream ss;
    for (int i = 0; i < consensus->_config.max_block_size; i++) {
      ss << "a";
    }
    tagged_block.mutable_block()
        ->mutable_coinbase()
        ->mutable_outputs(0)
        ->set_data(ss.str());
    ASSERT_FALSE(consensus->check_block_size(tagged_block));
  }

  void test_check_block_timestamp() {
    messages::TaggedBlock last_block, tagged_block;
    ASSERT_TRUE(ledger->get_last_block(&last_block));
    auto block = simulator.new_block(2, last_block);
    tagged_block.mutable_block()->CopyFrom(block);
    ASSERT_TRUE(consensus->check_block_timestamp(tagged_block));

    auto timestamp = tagged_block.block().header().timestamp().data();
    tagged_block.mutable_block()
        ->mutable_header()
        ->mutable_timestamp()
        ->set_data(timestamp + consensus->_config.block_period + 1);
    ASSERT_FALSE(consensus->check_block_timestamp(tagged_block));
  }

  void test_check_block_transactions() {
    messages::TaggedBlock last_block, tagged_block;
    ASSERT_TRUE(ledger->get_last_block(&last_block));
    auto block = simulator.new_block(2, last_block);
    ledger->insert_block(block);
    ledger->get_block(block.header().id(), &tagged_block);
    ASSERT_TRUE(consensus->check_block_transactions(tagged_block));

    block = simulator.new_block(2, last_block);
    ledger->insert_block(block);
    ledger->get_block(block.header().id(), &tagged_block);
    messages::NCCValue value =
        tagged_block.block().transactions(0).outputs(0).value().value();
    tagged_block.mutable_block()
        ->mutable_transactions(0)
        ->mutable_outputs(0)
        ->mutable_value()
        ->set_value(value + 1);
    ASSERT_FALSE(consensus->check_block_transactions(tagged_block));
  }

  void test_check_block_height() {}

  void test_check_block_author() {}
};

TEST_F(Consensus, is_valid_transaction) { test_is_valid_transaction(); }

TEST_F(Consensus, is_valid_block) { test_is_valid_block(); }

TEST_F(Consensus, get_current_height) {
  messages::Block block0;
  ASSERT_TRUE(ledger->get_block(0, &block0));
  auto height = consensus->get_current_height();
  ASSERT_TRUE(
      abs(height - ((std::time(nullptr) - block0.header().timestamp().data()) /
                    consensus->config().block_period)) <= 1);
}

TEST_F(Consensus, add_block) {
  messages::TaggedBlock block0;
  ASSERT_TRUE(ledger->get_block(0, &block0));
  auto block = simulator.new_block(10, block0);
  ASSERT_TRUE(consensus->add_block(block));
}

TEST_F(Consensus, compute_assembly_pii) { test_compute_assembly_pii(); }

TEST_F(Consensus, start_computations) { test_start_computations(); }

TEST_F(Consensus, add_transaction) {
  auto t0 = ledger->send_ncc(simulator.keys[0].key_priv(),
                             simulator.addresses[1], 0.5);
  auto t1 = ledger->send_ncc(simulator.keys[0].key_priv(),
                             simulator.addresses[0], 0.5);
  ASSERT_TRUE(consensus->add_transaction(t0));

  // The second time the transaction should be valid but not inserted twice
  ASSERT_FALSE(consensus->add_transaction(t0));

  // t1 is also invalid because it tries to double spend
  ASSERT_FALSE(consensus->add_transaction(t1));

  // Now that t0 has been added to the transaction pool. t2 should be build with
  // a different input so there should be no double spending
  auto t2 = ledger->send_ncc(simulator.keys[0].key_priv(),
                             simulator.addresses[0], 0.5);

  ASSERT_TRUE(consensus->add_transaction(t2));
}

TEST_F(Consensus, build_block) { test_build_block(); }

TEST_F(Consensus, add_denunciations) {
  // Let's make the first miner double mine
  auto block1 = simulator.new_block();
  auto block1_bis = simulator.new_block(1);
  ASSERT_TRUE(simulator.consensus->add_block(block1));
  ASSERT_TRUE(simulator.consensus->add_block(block1_bis));
  messages::TaggedBlock tagged_block1;
  bool include_transactions = false;
  ASSERT_TRUE(ledger->get_last_block(&tagged_block1, include_transactions));
  ASSERT_EQ(tagged_block1.block().header().id(), block1.header().id());
  auto block = simulator.new_block();
  ASSERT_EQ(block.denunciations_size(), 1);
  auto denunciation = block.denunciations(0);
  ASSERT_EQ(denunciation.block_id(), block1_bis.header().id());
  ASSERT_EQ(denunciation.block_author(), block1_bis.header().author());
  ASSERT_EQ(denunciation.block_height(), block1_bis.header().height());
}

TEST_F(Consensus, money_supply) { test_money_supply(); }

TEST_F(Consensus, send_ncc) { test_send_ncc(); }

TEST_F(Consensus, fake_signature) {
  auto fees = messages::NCCAmount(100);
  auto transaction = ledger->send_ncc(simulator.keys[0].key_priv(),
                                      simulator.addresses[1], 0.5, fees);
  auto data =
      transaction.mutable_signatures(0)->mutable_signature()->mutable_data();
  (*data)[0] += 1;
  ASSERT_FALSE(consensus->add_transaction(transaction));
}

TEST_F(Consensus, fees_on_coinbase) {
  messages::Assembly assembly;
  ledger->get_assembly(-2, &assembly);
  messages::Address address;
  ASSERT_TRUE(consensus->get_block_writer(assembly, 1, &address));
  int address_index = -1;
  for (size_t i = 0; i < simulator.addresses.size(); i++) {
    if (address == simulator.addresses[i]) {
      address_index = i;
      break;
    }
  }
  ASSERT_NE(address_index, -1);
  const auto &keys = simulator.keys[address_index];
  auto block = simulator.new_block();
  auto coinbase = block.mutable_coinbase();
  coinbase->mutable_fees()->set_value(100);
  messages::set_transaction_hash(coinbase);
  messages::set_block_hash(&block);
  crypto::sign(keys, &block);
  ASSERT_FALSE(consensus->add_block(block));
}

TEST_F(Consensus, change_data) {
  auto fees = messages::NCCAmount(100);
  auto transaction = ledger->send_ncc(simulator.keys[0].key_priv(),
                                      simulator.addresses[1], 0.5, fees);
  transaction.mutable_outputs(0)->set_data("totoro");
  std::vector<const crypto::Ecc *> keys = {&simulator.keys[0]};
  LOG_DEBUG << "TRANSACTION " << transaction;
  crypto::sign(keys, &transaction);
  messages::set_transaction_hash(&transaction);
  LOG_DEBUG << "TRANSACTION " << transaction;
  ASSERT_TRUE(consensus->add_transaction(transaction));
}

TEST_F(Consensus, outputs_too_high) {
  auto fees = messages::NCCAmount(100);
  auto transaction = ledger->send_ncc(simulator.keys[0].key_priv(),
                                      simulator.addresses[1], 1, fees);
  auto amount = transaction.outputs(0).value().value();
  transaction.mutable_outputs(0)->mutable_value()->set_value(amount + 1);
  std::vector<const crypto::Ecc *> keys = {&simulator.keys[0]};
  crypto::sign(keys, &transaction);
  messages::set_transaction_hash(&transaction);
  ASSERT_FALSE(consensus->add_transaction(transaction));
}

TEST_F(Consensus, fees_too_high) {
  auto fees = messages::NCCAmount(100);
  auto transaction = ledger->send_ncc(simulator.keys[0].key_priv(),
                                      simulator.addresses[1], 1, fees);
  transaction.mutable_fees()->CopyFrom(messages::NCCAmount(101));
  std::vector<const crypto::Ecc *> keys = {&simulator.keys[0]};
  crypto::sign(keys, &transaction);
  messages::set_transaction_hash(&transaction);
  ASSERT_FALSE(consensus->add_transaction(transaction));
}

TEST_F(Consensus, transaction_overflow) {
  auto fees = messages::NCCAmount(-1);
  auto did_throw = false;
  try {
    auto transaction = ledger->send_ncc(simulator.keys[0].key_priv(),
                                        simulator.addresses[1], 1, fees);
  } catch (std::runtime_error) {
    did_throw = true;
  }
  ASSERT_TRUE(did_throw);

  fees = messages::NCCAmount(100);
  auto transaction = ledger->send_ncc(simulator.keys[0].key_priv(),
                                      simulator.addresses[1], 1, fees);
  transaction.mutable_outputs(0)->mutable_value()->set_value(-1);
  std::vector<const crypto::Ecc *> keys = {&simulator.keys[0]};
  crypto::sign(keys, &transaction);
  messages::set_transaction_hash(&transaction);
  ASSERT_FALSE(consensus->add_transaction(transaction));
}

TEST_F(Consensus, check_transaction_id) { test_check_transaction_id(); }

TEST_F(Consensus, check_transaction_signatures) {
  test_check_transaction_signatures();
}

TEST_F(Consensus, check_transaction_outputs) {
  test_check_transaction_outputs();
}

TEST_F(Consensus, check_transaction_inputs) { test_check_transaction_inputs(); }

TEST_F(Consensus, check_transaction_double_inputs) {
  test_check_transaction_double_inputs();
}

TEST_F(Consensus, check_coinbase) { test_check_coinbase(); }

TEST_F(Consensus, check_transactions_order) { test_check_transactions_order(); }

TEST_F(Consensus, check_block_id) { test_check_block_id(); }

TEST_F(Consensus, check_block_size) { test_check_block_size(); }

TEST_F(Consensus, check_block_timestamp) { test_check_block_timestamp(); }

TEST_F(Consensus, check_block_transactions) { test_check_block_transactions(); }

TEST_F(Consensus, check_block_height) { test_check_block_height(); }

TEST_F(Consensus, check_block_author) { test_check_block_author(); }

}  // namespace tests
}  // namespace consensus
}  // namespace neuro
