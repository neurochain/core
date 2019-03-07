#include "tooling/blockgen.hpp"
#include <fstream>
#include <iostream>
#include "common/logger.hpp"
#include "crypto/Sign.hpp"

namespace po = boost::program_options;

namespace neuro {
namespace tooling {
namespace blockgen {

void coinbase(const crypto::EccPub &key_pub, const messages::NCCAmount &ncc,
              messages::Transaction *transaction,
              const messages::BlockHeight &height,
              const std::string output_data) {
  Buffer key_pub_raw;
  key_pub.save(&key_pub_raw);
  messages::Address address(key_pub_raw);
  auto output = transaction->add_outputs();
  output->mutable_address()->CopyFrom(address);
  output->mutable_value()->CopyFrom(ncc);
  output->set_data(output_data);
  transaction->set_coinbase_height(height);

  messages::set_transaction_hash(transaction);
}

messages::TaggedBlock gen_block0(const std::vector<crypto::Ecc> &keys,
                                 const messages::NCCAmount &ncc_block0,
                                 int32_t time_delta) {
  messages::TaggedBlock tagged_block;
  auto block = tagged_block.mutable_block();
  auto header = block->mutable_header();
  header->mutable_timestamp()->set_data(std::time(nullptr) + time_delta);
  auto previons_block_hash = header->mutable_previous_block_hash();
  previons_block_hash->set_data("");
  previons_block_hash->set_type(messages::Hash::Type::Hash_Type_SHA256);
  header->set_height(0);
  for (const auto &key : keys) {
    auto transaction = block->add_coinbases();
    blockgen::coinbase(key.public_key(), ncc_block0, transaction, 0);
  }
  tagged_block.set_branch(messages::Branch::MAIN);
  tagged_block.mutable_branch_path()->add_branch_ids(0);
  tagged_block.mutable_branch_path()->add_block_numbers(0);
  tagged_block.set_score(0);
  messages::sort_transactions(tagged_block.mutable_block());
  messages::set_block_hash(tagged_block.mutable_block());
  crypto::sign(keys[0], tagged_block.mutable_block());
  return tagged_block;
}

void block0(uint32_t bots, const std::string &pathdir,
            const messages::NCCAmount &nccsdf, ledger::LedgerMongodb *ledger) {
  std::vector<crypto::Ecc> keys;
  for (uint32_t i = 0; i < bots; ++i) {
    // gen i keys
    crypto::Ecc &ecc = keys.emplace_back();
    ecc.save({pathdir + "/key_" + std::to_string(i) + ".priv"},
             {pathdir + "/key_" + std::to_string(i) + ".pub"});
  }

  auto tagged_block0 = gen_block0(keys, nccsdf);
  ledger->insert_block(tagged_block0);

  std::ofstream blockfile0;
  blockfile0.open("block.0.bp");
  blockfile0 << tagged_block0.block().SerializeAsString();
  blockfile0.close();
}

void testnet_blockg(uint32_t bots, const std::string &pathdir,
                    messages::NCCAmount &nccsdf) {
  messages::Block blockfaucet;

  crypto::Ecc ecc;
  ecc.save({pathdir + "/key_faucet.priv"}, {pathdir + "/key_faucet.pub"});

  crypto::Ecc ecc_save;
  ecc_save.save({pathdir + "/key_faucet_save.priv"},
                {pathdir + "/key_faucet_save.pub"});

  Buffer key_pub_raw_save = ecc_save.mutable_public_key()->save();

  messages::BlockHeader *header = blockfaucet.mutable_header();

  auto previons_block_hash = header->mutable_previous_block_hash();
  previons_block_hash->set_data("");  ///!< load 0
  previons_block_hash->set_type(messages::Hash::Type::Hash_Type_SHA256);

  header->mutable_timestamp()->set_data(std::time(nullptr) +
                                        3600);  // 1539640800);
  header->set_height(0);

  messages::Transaction *transaction = blockfaucet.add_coinbases();
  coinbase(ecc.public_key(), nccsdf, transaction, 0, "trax killed me");

  messages::Transaction *transaction2 = blockfaucet.add_coinbases();
  coinbase(ecc_save.public_key(), nccsdf, transaction2, 0, "what olivier ?");

  neuro::Buffer t23("riados");
  messages::Hasher hash_id_tmp(t23);
  header->mutable_id()->CopyFrom(hash_id_tmp);

  messages::sort_transactions(&blockfaucet);
  crypto::sign(ecc, &blockfaucet);
  messages::set_block_hash(&blockfaucet);

  std::cout << "block0 " << blockfaucet << std::endl;
  std::ofstream blockfile0;
  blockfile0.open("data.0.testnet");
  blockfile0 << blockfaucet.SerializeAsString();
  blockfile0.close();
}

bool blockgen_from_block(messages::Block *block,
                         const messages::Block &last_block,
                         const int32_t height, const uint64_t seed,
                         std::optional<neuro::messages::KeyPub> author) {
  /*uint32_t height = last_height;
  if (height == 0) {
    height = ledger->height();
  }
  neuro::messages::Block last_block;
  if (!ledger->get_block(height, &last_block)) {
    return false;
  }*/

  neuro::messages::BlockHeader *header = block->mutable_header();

  if (author) {
    header->mutable_author()->mutable_key_pub()->CopyFrom(*author);
    auto signature = header->mutable_author()->mutable_signature();
    signature->set_data("");
  } else {
    header->mutable_author()->CopyFrom(last_block.header().author());
  }

  header->mutable_previous_block_hash()->CopyFrom(last_block.header().id());
  header->mutable_timestamp()->set_data(last_block.header().timestamp().data() +
                                        15);
  header->set_height(height);

  // DO Transaction
  std::srand(seed);
  std::vector<int> used_transaction;
  int num_last_transaction = last_block.transactions_size();
  int num_transaction = num_last_transaction;

  for (int i = 0; i < num_transaction; ++i) {
    const neuro::messages::Transaction &sender = last_block.transactions(i);
    const neuro::messages::Transaction &recevied = last_block.transactions(i);

    int32_t num_output = 0;  // To use rand
    uint64_t total_ncc =
        sender.outputs(num_output)
            .value()
            .value();  // std::atol(sender.outputs(num_output).value().value().c_str());

    neuro::messages::Transaction *new_trans = block->add_transactions();

    // Input from sender
    neuro::messages::Input *input = new_trans->add_inputs();
    input->mutable_id()->CopyFrom(sender.id());
    input->set_output_id(num_output);
    input->set_signature_id(i);

    // Output to recevied
    // Output to sender with min 1 ncc
    neuro::messages::Output *output_revevied = new_trans->add_outputs();
    output_revevied->mutable_address()->CopyFrom(
        recevied.outputs(num_output).address());
    output_revevied->mutable_value()->set_value(total_ncc);

    messages::set_transaction_hash(new_trans);
  }

  messages::sort_transactions(block);
  messages::set_block_hash(block);
  return true;
}

bool blockgen_from_last_db_block(messages::Block *block,
                                 std::shared_ptr<ledger::Ledger> ledger,
                                 const uint64_t seed, const int32_t new_height,
                                 std::optional<neuro::messages::KeyPub> author,
                                 const int32_t last_height) {
  int32_t height = last_height;
  if (height == 0) {
    height = ledger->height();
  }
  neuro::messages::Block last_block;
  ledger->get_block(height, &last_block);

  if (last_block.header().height() != height) {
    throw std::runtime_error({"Not found block 2 - " + std::to_string(height) +
                              " - " +
                              std::to_string(last_block.header().height())});
  }

  return blockgen_from_block(block, last_block, new_height, seed, author);
}

void append_blocks(const int nb, std::shared_ptr<ledger::Ledger> ledger) {
  messages::Block last_block;
  ledger->get_block(ledger->height(), &last_block);
  for (int i = 0; i < nb; ++i) {
    messages::Block block;
    blockgen_from_last_db_block(&block, ledger, 1, i + 1);
    messages::TaggedBlock tagged_block;
    tagged_block.set_branch(messages::Branch::MAIN);
    tagged_block.mutable_branch_path()->add_branch_ids(0);
    tagged_block.mutable_branch_path()->add_block_numbers(0);
    *tagged_block.mutable_block() = block;
    ledger->insert_block(tagged_block);
  }
}

void append_fork_blocks(const int nb, std::shared_ptr<ledger::Ledger> ledger) {
  messages::TaggedBlock last_block, tagged_block;
  ledger->get_block(ledger->height(), &last_block);
  for (int i = 0; i < nb; ++i) {
    blockgen_from_block(tagged_block.mutable_block(), last_block.block(),
                        last_block.block().header().height() + 1);
    tagged_block.set_branch(messages::Branch::FORK);
    messages::BranchPath branch_path;
    if (i == 0) {
      branch_path = ledger->fork_from(last_block.branch_path());
    } else {
      branch_path = ledger->first_child(last_block.branch_path());
    }
    tagged_block.mutable_branch_path()->CopyFrom(branch_path);
    tagged_block.mutable_branch_path()->add_block_numbers(0);
    ledger->insert_block(tagged_block);
    last_block.CopyFrom(tagged_block);
    tagged_block.Clear();
  }
}

void create_empty_db(const messages::config::Database &config) {
  // instance started in static in LedgerMongodb
  mongocxx::uri uri(config.url());
  mongocxx::client client(uri);
  mongocxx::database db(client[config.db_name()]);
  db.drop();
}

}  // namespace blockgen
}  // namespace tooling
}  // namespace neuro
