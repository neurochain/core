#include "crypto/Ecc.hpp"
#include "ledger/LedgerMongodb.hpp"
#include "tooling/blockgen.hpp"

namespace neuro {
namespace tooling {

class Simulator {
 private:
  messages::NCCSDF ncc_block0;
  messages::NCCSDF block_reward;

 public:
  std::shared_ptr<neuro::ledger::LedgerMongodb> ledger;
  std::vector<crypto::Ecc> keys;
  std::vector<messages::Address> addresses;

  Simulator(const std::string &url, const std::string &db_name,
            const int nb_keys, const messages::NCCSDF ncc_block0,
            const messages::NCCSDF block_reward)
      : ncc_block0(ncc_block0),
        block_reward(block_reward),
        ledger(std::make_shared<ledger::LedgerMongodb>(url, db_name)),
        keys(nb_keys) {
    ledger->empty_database();
    auto tagged_block = gen_block0();
    ledger->insert_block(&tagged_block);
    for (const auto &key : keys) {
      addresses.push_back(key.public_key());
    }
  }

  messages::TaggedBlock gen_block0();
  messages::Block new_block(int nb_transactions);
  messages::Transaction new_random_transaction();
  messages::Transaction send_ncc(const crypto::EccPriv &from_key_priv,
                                 const messages::Address &to_address,
                                 float ncc_ration);
  messages::Transaction random_transaction();
  void run(int nb_blocks, int transactions_per_block);
};

messages::TaggedBlock Simulator::gen_block0() {
  messages::TaggedBlock tagged_block;
  auto block = tagged_block.mutable_block();
  auto header = block->mutable_header();
  keys[0].public_key().save(header->mutable_author());
  header->mutable_timestamp()->set_data(time(0));
  auto previons_block_hash = header->mutable_previous_block_hash();
  previons_block_hash->set_data("");
  previons_block_hash->set_type(messages::Hash::Type::Hash_Type_SHA256);
  header->set_height(0);
  for (const auto &key : keys) {
    auto transaction = block->add_transactions();
    blockgen::coinbase(key.public_key(), ncc_block0, transaction, "");
  }
  tagged_block.set_branch(messages::Branch::MAIN);
  tagged_block.mutable_branch_path()->add_branch_ids(0);
  tagged_block.mutable_branch_path()->add_block_numbers(0);
  messages::set_block_hash(tagged_block.mutable_block());
  return tagged_block;
}

messages::Transaction Simulator::send_ncc(const crypto::EccPriv &from_key_priv,
                                          const messages::Address &to_address,
                                          float ncc_ratio) {
  const auto from_address = messages::Address(from_key_priv.make_public_key());
  const auto available_ncc = ledger->available_ncc(from_address);
  auto amount = messages::ncc_amount(ncc_ratio * available_ncc.value());
  auto transaction =
      ledger->build_transaction(to_address, amount, from_key_priv);
  return transaction;
}

messages::Transaction Simulator::random_transaction() {
  int sender_index = rand() % keys.size();
  int recipient_index = rand() % keys.size();
  return send_ncc(keys[sender_index].private_key(), addresses[recipient_index],
                  0.5);
}

messages::Block Simulator::new_block(int nb_transactions) {
  messages::Block last_block, block;
  ledger->get_block(ledger->height(), &last_block);
  int miner_index = rand() % keys.size();
  auto header = block.mutable_header();
  keys[miner_index].public_key().save(header->mutable_author());
  header->mutable_timestamp()->set_data(time(0));
  header->mutable_previous_block_hash()->CopyFrom(last_block.header().id());
  header->set_height(last_block.header().height() + 1);

  // Block reward
  auto transaction = block.add_transactions();
  blockgen::coinbase(keys[miner_index].public_key(), block_reward, transaction);

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
