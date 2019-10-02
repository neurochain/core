#include <assert.h>

#include "common.pb.h"
#include "consensus.pb.h"
#include "consensus/Config.hpp"
#include "messages/Message.hpp"
#include "tooling/Simulator.hpp"

namespace neuro {
namespace tooling {

static consensus::Config config{
    .blocks_per_assembly = 5,
    .members_per_assembly = 10,
    .block_period = 1,
    .block_reward = messages::NCCAmount{uint64_t{100}},
    .max_block_size = 256000,
    .max_transaction_per_block = 300,
    .update_heights_sleep = 1s,
    .compute_pii_sleep = 1s,
    .miner_sleep = 100ms,
    .integrity_block_reward = 1,
    .integrity_double_mining = -40,
    .integrity_denunciation_reward = 1,
    .default_transaction_expires = 5760};

Simulator::Simulator(const std::string &db_url, const std::string &db_name,
                     const int32_t nb_keys,
                     const messages::NCCAmount &ncc_block0,
                     const int32_t time_delta, const bool start_threads)
    : _ncc_block0(ncc_block0),
      keys(nb_keys),
      ledger(std::make_shared<ledger::LedgerMongodb>(
          db_url, db_name,
          tooling::blockgen::gen_block0(keys, _ncc_block0, time_delta)
              .block())),
      consensus(std::make_shared<consensus::Consensus>(
          ledger, keys, config, [](const messages::Block &block) {},
          start_threads)) {
  for (size_t i = 0; i < keys.size(); i++) {
    auto &key_pub = key_pubs.emplace_back(keys[i].key_pub());
    key_pubs_indexes.insert({key_pub, i});
  }
}

Simulator Simulator::RealtimeSimulator(const std::string &db_url,
                                       const std::string &db_name,
                                       const int nb_keys,
                                       const messages::NCCAmount ncc_block0) {
  // Put the block0 2 seconds in the future so that we have time to create the
  // database and still be ready to write block 1
  bool start_threads = true;
  return Simulator(db_url, db_name, nb_keys, ncc_block0, 2, start_threads);
}

Simulator Simulator::StaticSimulator(const std::string &db_url,
                                     const std::string &db_name,
                                     const int nb_keys,
                                     const messages::NCCAmount ncc_block0) {
  bool start_threads = false;
  return Simulator(db_url, db_name, nb_keys, ncc_block0, -100000,
                   start_threads);
}

messages::Transaction Simulator::random_transaction() const {
  int sender_index = rand() % keys.size();
  int recipient_index = rand() % keys.size();
  return ledger->send_ncc(keys[sender_index].key_priv(),
                          key_pubs[recipient_index], RATIO_TO_SEND,
                          messages::NCCAmount(1));
}

/*
 * Create a new block just after the last block passed.
 * It will include transactions in the transaction pool and also generate
 * nb_transactions random transactions.
 */
messages::Block Simulator::new_block(
    int nb_transactions, const messages::TaggedBlock &last_block) const {
  messages::Block block, block0;
  ledger->get_block(0, &block0);

  for (int i = 0; i < nb_transactions; i++) {
    auto transaction = random_transaction();
    messages::TaggedTransaction tagged_transaction;
    tagged_transaction.mutable_transaction()->CopyFrom(transaction);
    tagged_transaction.set_is_coinbase(false);
    ledger->add_transaction(tagged_transaction);
  }

  messages::Assembly assembly_n_minus_1, assembly_n_minus_2;
  assert(ledger->get_assembly(last_block.previous_assembly_id(),
                              &assembly_n_minus_1));
  assert(ledger->get_assembly(assembly_n_minus_1.previous_assembly_id(),
                              &assembly_n_minus_2));
  auto height = last_block.block().header().height() + 1;
  auto &assembly = height % consensus->config().blocks_per_assembly == 0
                       ? assembly_n_minus_1
                       : assembly_n_minus_2;

  messages::_KeyPub key_pub;
  assert(consensus->get_block_writer(assembly, height, &key_pub));
  uint32_t miner_index = key_pubs_indexes.at(key_pub);
  auto header = block.mutable_header();
  header->mutable_timestamp()->set_data(block0.header().timestamp().data() +
                                        height *
                                            consensus->config().block_period);
  header->mutable_previous_block_hash()->CopyFrom(
      last_block.block().header().id());
  header->set_height(height);

  ledger->get_transaction_pool(&block, config.max_block_size,
                               config.max_transaction_per_block);
  consensus->cleanup_transactions(&block);

  messages::NCCValue total_fees = 0;
  for (const auto &transaction : block.transactions()) {
    if (transaction.has_fees()) {
      total_fees += transaction.fees().value();
    }
  }

  auto block_reward = messages::NCCAmount(
      consensus->config().block_reward.value() + total_fees);

  // Block reward
  blockgen::coinbase({keys[miner_index].key_pub()}, block_reward,
                     block.mutable_coinbase(),
                     last_block.block().header().id());

  ledger->add_denunciations(&block, last_block.branch_path());
  messages::sort_transactions(&block);
  messages::set_block_hash(&block);
  crypto::sign(keys[miner_index], &block);
  return block;
}

/*
 * Create a new block just after the last block passed.
 * It will include transactions in the transaction pool.
 */
messages::Block Simulator::new_block(
    const messages::TaggedBlock &last_block) const {
  return new_block(0, last_block);
}

/*
 * Let's fetch the last block automatically and create a new one
 */
messages::Block Simulator::new_block(int nb_transactions) const {
  messages::TaggedBlock last_block;
  if (!ledger->get_last_block(&last_block)) {
    throw std::runtime_error("Failed to get the last block in the simulator");
  }
  return new_block(nb_transactions, last_block);
}

/*
 * Let's fetch the last block automatically and create a new one
 */
messages::Block Simulator::new_block() const {
  messages::TaggedBlock last_block;
  if (!ledger->get_last_block(&last_block)) {
    throw std::runtime_error("Failed to get the last block in the simulator");
  }
  return new_block(0, last_block);
}

void Simulator::run(int nb_blocks, int transactions_per_block,
                    bool compute_pii) {
  messages::TaggedBlock last_block;
  for (int i = 0; i < nb_blocks; i++) {
    bool include_transactions = false;
    ledger->get_last_block(&last_block, include_transactions);
    auto block = new_block(transactions_per_block, last_block);
    if (!consensus->add_block(block)) {
      throw std::runtime_error("Failed to add block in the simulator");
    }
    if (compute_pii && (i - 1) % consensus->config().blocks_per_assembly == 0) {
      std::vector<messages::Assembly> assemblies;
      ledger->get_assemblies_to_compute(&assemblies);
      if (assemblies.size() > 0) {
        consensus->compute_assembly_pii(assemblies.at(0));
      }
    }
  }
}

}  // namespace tooling
}  // namespace neuro
