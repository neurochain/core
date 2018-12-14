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
  }

  messages::TaggedBlock gen_block0();
  messages::Block new_block(const messages::Hash &previous_block_id);
  messages::Transaction new_transaction(const messages::Hash &last_block_id);
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

messages::Block Simulator::new_block(const messages::Hash &previous_block_id) {
  messages::Block block;
  return block;
}

messages::Transaction Simulator::new_transaction(
    const messages::Hash &last_block_id) {
  messages::Transaction transaction;
  return transaction;
}

}  // namespace tooling
}  // namespace neuro
