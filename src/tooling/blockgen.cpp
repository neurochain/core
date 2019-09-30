#include "tooling/blockgen.hpp"
#include "crypto/Ecc.hpp"
#include "messages/Message.hpp"
#include "mongocxx/client.hpp"
#include "mongocxx/database.hpp"
#include "mongocxx/uri.hpp"

namespace po = boost::program_options;

namespace neuro {
namespace tooling {
namespace blockgen {

void coinbase(const std::vector<crypto::KeyPub> &pub_keys,
              const messages::NCCAmount &ncc,
              messages::Transaction *transaction,
              const messages::BlockID &last_seen_block_id,
              const std::string &output_data) {
  for (const auto &pub_key : pub_keys) {
    auto output = transaction->add_outputs();
    output->mutable_key_pub()->CopyFrom(messages::_KeyPub(pub_key));
    output->mutable_value()->CopyFrom(ncc);
    output->set_data(output_data);
  }
  transaction->mutable_last_seen_block_id()->CopyFrom(last_seen_block_id);

  messages::set_transaction_hash(transaction);
}

void coinbase(const std::vector<crypto::Ecc> &eccs,
              const messages::NCCAmount &ncc,
              messages::Transaction *transaction,
              const messages::BlockID &last_seen_block_id,
              const std::string &output_data) {
  for (size_t i = 0; i < eccs.size(); i++) {
    auto output = transaction->add_outputs();
    output->mutable_key_pub()->CopyFrom(messages::_KeyPub(eccs[i].key_pub()));
    output->mutable_value()->CopyFrom(ncc);
    output->set_data(output_data);
  }
  transaction->mutable_last_seen_block_id()->CopyFrom(last_seen_block_id);

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
  header->set_height(0);
  std::vector<crypto::KeyPub> pub_keys;
  for (const auto &key : keys) {
    pub_keys.push_back(key.key_pub());
  }

  messages::BlockID block_id;
  block_id.set_data("");
  blockgen::coinbase(pub_keys, ncc_block0, block->mutable_coinbase(), block_id);
  tagged_block.set_branch(messages::Branch::MAIN);
  tagged_block.mutable_branch_path()->add_branch_ids(0);
  tagged_block.mutable_branch_path()->add_block_numbers(0);
  tagged_block.mutable_branch_path()->set_branch_id(0);
  tagged_block.mutable_branch_path()->set_block_number(0);
  tagged_block.set_score(0);
  messages::sort_transactions(tagged_block.mutable_block());
  messages::set_block_hash(tagged_block.mutable_block());
  crypto::sign(keys[0], tagged_block.mutable_block());
  return tagged_block;
}

std::vector<crypto::Ecc> create_key_pairs(uint32_t number_of_wallets,
                                          const Path &pathdir) {
  std::vector<crypto::Ecc> eccs;
  for (uint32_t i = 0; i < number_of_wallets; i++) {
    const auto key_priv_name = "key" + std::to_string(i) + ".priv";
    const auto key_pub_name = "key" + std::to_string(i) + ".pub";
    eccs.emplace_back(pathdir / key_priv_name, pathdir / key_pub_name);
  }
  return eccs;
}

void testnet_blockg(uint32_t number_of_wallets, const Path &pathdir,
                    messages::NCCAmount &nccsdf) {
  const auto eccs = create_key_pairs(number_of_wallets, pathdir);
  const messages::Block block0 = gen_block0(eccs, nccsdf, 0).block();

  std::cout << "block0 " << block0 << std::endl;
  std::ofstream stream_block0;
  stream_block0.open("data.0.testnet");
  stream_block0 << block0.SerializeAsString();
  stream_block0.close();
}

bool blockgen_from_block(
    messages::Block *block, const messages::Block &last_block,
    const int32_t height, const uint64_t seed,
    const std::optional<neuro::messages::_KeyPub> &author) {
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
    input->mutable_key_pub()->CopyFrom(sender.outputs(0).key_pub());
    input->mutable_value()->set_value(total_ncc);

    // Output to recevied
    // Output to sender with min 1 ncc
    neuro::messages::Output *output_revevied = new_trans->add_outputs();
    output_revevied->mutable_key_pub()->CopyFrom(
        recevied.outputs(num_output).key_pub());
    output_revevied->mutable_value()->set_value(total_ncc);

    new_trans->mutable_last_seen_block_id()->CopyFrom(last_block.header().id());

    messages::set_transaction_hash(new_trans);
  }

  messages::sort_transactions(block);
  messages::set_block_hash(block);
  return true;
}

bool blockgen_from_last_db_block(
    messages::Block *block, std::shared_ptr<ledger::Ledger> ledger,
    const uint64_t seed, const int32_t new_height,
    const std::optional<neuro::messages::_KeyPub> &author,
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
    tagged_block.mutable_branch_path()->add_block_numbers(i);
    tagged_block.mutable_branch_path()->set_branch_id(0);
    tagged_block.mutable_branch_path()->set_block_number(i);
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
    tagged_block.mutable_branch_path()->set_block_number(0);
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
