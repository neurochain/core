#include "tooling/Simulator.hpp"
#include "consensus/Config.hpp"
#include "tooling/blockgen.hpp"

namespace neuro {
namespace tooling {

consensus::Config config{
    10,     // blocks_per_assembly
    10,     // members_per_assembly
    15,     // block_period
    100,    // block_reward
    128000  // max_block_size
};

Simulator::Simulator(const std::string &db_url, const std::string &db_name,
                     const int nb_keys, const messages::NCCAmount ncc_block0,
                     const messages::NCCAmount block_reward)
    : _ncc_block0(ncc_block0),
      ledger(std::make_shared<ledger::LedgerMongodb>(db_url, db_name)),
      consensus(ledger, config),
      keys(nb_keys) {
  ledger->empty_database();
  for (const auto &key : keys) {
    addresses.push_back(key.public_key());
  }
  auto tagged_block = tooling::blockgen::gen_block0(keys, _ncc_block0);
  ledger->init_database(tagged_block.block());
  ledger->set_main_branch_tip();
}

messages::Transaction Simulator::send_ncc(const crypto::EccPriv &from_key_priv,
                                          const messages::Address &to_address,
                                          float ncc_ratio) {
  const auto from_address = messages::Address(from_key_priv.make_public_key());
  const auto balance = ledger->balance(from_address);
  auto amount = messages::NCCAmount(ncc_ratio * balance.value());
  std::cout << "ncc balance " << amount << std::endl;
  auto transaction =
      ledger->build_transaction(to_address, amount, from_key_priv);
  LOG_INFO << "transaction " << transaction << std::endl;
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
  messages::Block block;
  messages::Assembly assembly_n_minus_1, assembly_n_minus_2;
  ledger->get_assembly(last_block.previous_assembly_id(), &assembly_n_minus_1);
  auto height = last_block.block().header().height() + 1;
  auto assembly = height % consensus.config.blocks_per_assembly == 0
                      ? assembly_n_minus_1
                      : assembly_n_minus_2;

  int miner_index = rand() % keys.size();
  auto header = block.mutable_header();
  keys[miner_index].public_key().save(header->mutable_author());
  header->mutable_timestamp()->set_data(std::time(nullptr));
  header->mutable_previous_block_hash()->CopyFrom(
      last_block.block().header().id());
  header->set_height(last_block.block().header().height() + 1);

  // Block reward
  auto transaction = block.add_coinbases();
  blockgen::coinbase(keys[miner_index].public_key(),
                     consensus.config.block_reward, transaction);

  for (int i = 0; i < nb_transactions; i++) {
    auto transaction = random_transaction();
    messages::TaggedTransaction tagged_transaction;
    tagged_transaction.mutable_transaction()->CopyFrom(transaction);
    tagged_transaction.set_is_coinbase(false);
    ledger->add_transaction(tagged_transaction);
    block.add_transactions()->CopyFrom(transaction);
  }

  messages::sort_transactions(&block);
  messages::set_block_hash(&block);

  return block;
}

void Simulator::run(int nb_blocks, int transactions_per_block) {
  messages::TaggedBlock last_block;
  for (int i = 0; i < nb_blocks; i++) {
    ledger->get_last_block(&last_block);
    auto block = new_block(transactions_per_block, last_block);
    if (!consensus.add_block(&block)) {
      throw("Failed to add block in the simulator");
    }
  }
}

}  // namespace tooling
}  // namespace neuro
