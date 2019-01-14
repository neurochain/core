#include "tooling/Simulator.hpp"
#include "tooling/blockgen.hpp"

namespace neuro {
namespace tooling {

Simulator::Simulator(const std::string &db_url, const std::string &db_name,
                     const int nb_keys, const messages::NCCAmount ncc_block0,
                     const messages::NCCAmount block_reward)
    : _ncc_block0(ncc_block0),
      _block_reward(block_reward),
      ledger(std::make_shared<ledger::LedgerMongodb>(db_url, db_name)),
      keys(nb_keys) {
  ledger->empty_database();
  auto tagged_block = tooling::blockgen::gen_block0(keys, _ncc_block0);
  ledger->insert_block(&tagged_block);
  for (const auto &key : keys) {
    addresses.push_back(key.public_key());
  }
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

messages::Block Simulator::new_block(int nb_transactions) {
  messages::Block last_block, block;
  ledger->get_block(ledger->height(), &last_block);
  int miner_index = rand() % keys.size();
  auto header = block.mutable_header();
  keys[miner_index].public_key().save(header->mutable_author());
  header->mutable_timestamp()->set_data(std::time(nullptr));
  header->mutable_previous_block_hash()->CopyFrom(last_block.header().id());
  header->set_height(last_block.header().height() + 1);

  // Block reward
  auto transaction = block.add_transactions();
  blockgen::coinbase(keys[miner_index].public_key(), _block_reward,
                     transaction);

  for (int i = 0; i < nb_transactions; i++) {
    auto transaction = random_transaction();
    messages::TaggedTransaction tagged_transaction;
    tagged_transaction.mutable_transaction()->CopyFrom(transaction);
    ledger->add_transaction(tagged_transaction);
    block.add_transactions()->CopyFrom(transaction);
  }

  messages::set_block_hash(&block);

  return block;
}

void Simulator::run(int nb_blocks, int transactions_per_block) {
  for (int i = 0; i < nb_blocks; i++) {
    messages::TaggedBlock tagged_block;
    tagged_block.set_branch(messages::Branch::MAIN);
    tagged_block.mutable_branch_path()->add_branch_ids(0);
    tagged_block.mutable_branch_path()->add_block_numbers(0);
    tagged_block.mutable_block()->CopyFrom(new_block(transactions_per_block));
    ledger->insert_block(&tagged_block);
  }
}

}  // namespace tooling
}  // namespace neuro
