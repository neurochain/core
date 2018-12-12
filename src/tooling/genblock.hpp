#ifndef NEURO_SRC_TOOLING_GENBLOCK_HPP
#define NEURO_SRC_TOOLING_GENBLOCK_HPP

#include <boost/program_options.hpp>
#include "common/logger.hpp"
#include "common/types.hpp"
#include "crypto/Ecc.hpp"
#include "crypto/Hash.hpp"

#include "ledger/Ledger.hpp"
#include "messages.pb.h"
#include "messages/Hasher.hpp"
#include "messages/Message.hpp"

#include <fstream>
#include <iostream>
#include <optional>

namespace po = boost::program_options;

namespace neuro {
namespace tooling {
namespace genblock {

bool genblock_from_block(
    messages::Block *block, const messages::Block &last_block,
    const int32_t height, const uint64_t seed = 1,
    std::optional<neuro::messages::KeyPub> author = std::nullopt) {
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
    header->mutable_author()->CopyFrom(*author);
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
    input->mutable_block_id()->CopyFrom(last_block.header().id());
    input->set_key_id(0);

    // Output to recevied
    // Output to sender with min 1 ncc
    neuro::messages::Output *output_revevied = new_trans->add_outputs();
    output_revevied->mutable_address()->CopyFrom(
        recevied.outputs(num_output).address());
    output_revevied->mutable_value()->set_value(total_ncc);

    new_trans->mutable_fees()->set_value(0);
    messages::set_transaction_hash(new_trans);
  }

  messages::set_block_hash(block);
  return true;
}

bool genblock_from_last_db_block(
    messages::Block *block, std::shared_ptr<ledger::Ledger> ledger,
    const uint64_t seed, const int32_t new_height,
    std::optional<neuro::messages::KeyPub> author = std::nullopt,
    const int32_t last_height = 0) {
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

  return genblock_from_block(block, last_block, new_height, seed, author);
}

void append_blocks(const int nb, std::shared_ptr<ledger::Ledger> ledger) {
  messages::Block last_block;
  ledger->get_block(ledger->height(), &last_block);
  for (int i = 0; i < nb; ++i) {
    messages::Block block;
    genblock_from_last_db_block(&block, ledger, 1, i + 1);
    messages::TaggedBlock tagged_block;
    tagged_block.set_branch(messages::Branch::MAIN);
    tagged_block.mutable_branch_path()->add_branch_ids(0);
    tagged_block.mutable_branch_path()->add_block_numbers(0);
    *tagged_block.mutable_block() = block;
    ledger->insert_block(&tagged_block);
  }
}

void append_fork_blocks(const int nb, std::shared_ptr<ledger::Ledger> ledger) {
  messages::TaggedBlock last_block, tagged_block;
  ledger->get_block(ledger->height(), &last_block);
  for (int i = 0; i < nb; ++i) {
    genblock_from_block(tagged_block.mutable_block(), last_block.block(),
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
    ledger->insert_block(&tagged_block);
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

}  // namespace genblock
}  // namespace tooling
}  // namespace neuro

#endif
