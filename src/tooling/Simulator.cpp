#include "tooling/Simulator.hpp"
#include <chrono>
#include "consensus/Config.hpp"
#include "crypto/Sign.hpp"
#include "tooling/blockgen.hpp"

namespace neuro {
namespace tooling {

consensus::Config config{
    5,                               // blocks_per_assembly
    10,                              // members_per_assembly
    1,                               // block_period
    100,                             // block_reward
    128000,                          // max_block_size
    std::chrono::seconds(1),         // update_heights_sleep
    std::chrono::seconds(1),         // compute_pii_sleep
    std::chrono::milliseconds(100),  // miner_sleep
};

Simulator::Simulator(const std::string &db_url, const std::string &db_name,
                     const int32_t nb_keys,
                     const messages::NCCAmount &ncc_block0,
                     const int32_t time_delta)
    : _ncc_block0(ncc_block0),
      keys(nb_keys),
      ledger(std::make_shared<ledger::LedgerMongodb>(
          db_url, db_name,
          tooling::blockgen::gen_block0(keys, _ncc_block0, time_delta)
              .block())),
      consensus(std::make_shared<consensus::Consensus>(
          ledger, keys, config, [](const messages::Block &block) {})) {
  if (time_delta < 0) {
    // If the time_delta is in the past we have a StaticSimulator and it should
    // not mine new blocks
    consensus->_stop_update_heights = true;
    consensus->_stop_miner = true;
  }
  for (size_t i = 0; i < keys.size(); i++) {
    auto &address = addresses.emplace_back(keys[i].public_key());
    addresses_indexes.insert({address, i});
  }
}

Simulator Simulator::RealtimeSimulator(const std::string &db_url,
                                       const std::string &db_name,
                                       const int nb_keys,
                                       const messages::NCCAmount ncc_block0) {
  // Put the block0 2 seconds in the future so that we have time to create the
  // database and still be ready to write block 1
  return Simulator(db_url, db_name, nb_keys, ncc_block0, 2);
}

Simulator Simulator::StaticSimulator(const std::string &db_url,
                                     const std::string &db_name,
                                     const int nb_keys,
                                     const messages::NCCAmount ncc_block0) {
  return Simulator(db_url, db_name, nb_keys, ncc_block0, -100000);
}

messages::Transaction Simulator::send_ncc(const crypto::EccPriv &from_key_priv,
                                          const messages::Address &to_address,
                                          float ncc_ratio) {
  const auto from_address = messages::Address(from_key_priv.make_public_key());
  const auto balance = ledger->balance(from_address);
  auto amount = messages::NCCAmount(ncc_ratio * balance.value());
  auto transaction =
      ledger->build_transaction(to_address, amount, from_key_priv);
  return transaction;
}

messages::Transaction Simulator::random_transaction() {
  int sender_index = rand() % keys.size();
  int recipient_index = rand() % keys.size();
  return send_ncc(keys[sender_index].private_key(), addresses[recipient_index],
                  RATIO_TO_SEND);
}

messages::Block Simulator::new_block(int nb_transactions,
                                     const messages::TaggedBlock &last_block) {
  messages::Block block, block0;
  ledger->get_block(0, &block0);
  messages::Assembly assembly_n_minus_1, assembly_n_minus_2;
  assert(ledger->get_assembly(last_block.previous_assembly_id(),
                              &assembly_n_minus_1));
  assert(ledger->get_assembly(assembly_n_minus_1.previous_assembly_id(),
                              &assembly_n_minus_2));
  auto height = last_block.block().header().height() + 1;
  auto &assembly = height % consensus->config.blocks_per_assembly == 0
                       ? assembly_n_minus_1
                       : assembly_n_minus_2;

  messages::Address address;
  assert(consensus->get_block_writer(assembly, height, &address));
  uint32_t miner_index = addresses_indexes.at(address);
  auto header = block.mutable_header();
  header->mutable_timestamp()->set_data(block0.header().timestamp().data() +
                                        height *
                                            consensus->config.block_period);
  header->mutable_previous_block_hash()->CopyFrom(
      last_block.block().header().id());
  header->set_height(height);

  // Block reward
  auto transaction = block.add_coinbases();
  blockgen::coinbase(keys[miner_index].public_key(),
                     consensus->config.block_reward, transaction, height);

  for (int i = 0; i < nb_transactions; i++) {
    auto transaction = random_transaction();
    messages::TaggedTransaction tagged_transaction;
    tagged_transaction.mutable_transaction()->CopyFrom(transaction);
    tagged_transaction.set_is_coinbase(false);
    ledger->add_transaction(tagged_transaction);
  }

  ledger->get_transaction_pool(&block);
  messages::sort_transactions(&block);
  crypto::sign(keys[miner_index], &block);
  messages::set_block_hash(&block);

  return block;
}

void Simulator::run(int nb_blocks, int transactions_per_block) {
  messages::TaggedBlock last_block;
  for (int i = 0; i < nb_blocks; i++) {
    bool include_transactions = false;
    ledger->get_last_block(&last_block, include_transactions);
    auto block = new_block(transactions_per_block, last_block);
    if (!consensus->add_block(&block)) {
      throw std::runtime_error("Failed to add block in the simulator");
    }
  }
}

}  // namespace tooling
}  // namespace neuro
