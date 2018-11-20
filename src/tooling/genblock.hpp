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
    messages::Block &block, messages::Block &last_block, const uint64_t seed,
    int32_t height,
    std::optional<neuro::messages::KeyPub> author = std::nullopt,
    const int max_trx = 20, const int max_trail = 5) {
  /*uint32_t height = last_height;
  if (height == 0) {
    height = ledger->height();
  }
  neuro::messages::Block last_block;
  if (!ledger->get_block(height, &last_block)) {
    return false;
  }*/

  neuro::messages::BlockHeader *header = block.mutable_header();

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
  int num_transaction = std::max(std::rand(), max_trx);
  int num_last_transaction = last_block.transactions_size();

  for (int i = 0; i < num_transaction; ++i) {
    int how_trial = 0;
    int rinput = std::rand() % num_last_transaction,
        routput = std::rand() % num_last_transaction;

    while (std::find(used_transaction.begin(), used_transaction.end(),
                     rinput) != used_transaction.end()) {
      rinput = std::rand() % num_last_transaction;
      how_trial++;
      if (how_trial > max_trail) break;
    }
    if (how_trial > max_trail) break;

    how_trial = 0;
    used_transaction.push_back(rinput);

    while (routput >= last_block.transactions_size()) {
      routput = std::rand() % num_last_transaction;
      how_trial++;
      if (how_trial > max_trail) break;
    }
    if (how_trial > max_trail) break;

    const neuro::messages::Transaction &sender =
        last_block.transactions(rinput);
    const neuro::messages::Transaction &recevied =
        last_block.transactions(routput);

    int32_t num_output = 0;  // To use rand
    uint64_t total_ncc =
        sender.outputs(num_output)
            .value()
            .value();  // std::atol(sender.outputs(num_output).value().value().c_str());
    if (total_ncc < 2) {
      break;
    }
    uint64_t how_ncc =
        std::min(total_ncc - 1, std::abs(std::rand()) % total_ncc);
    if (how_ncc == 0) {
      break;
    }

    neuro::messages::Transaction *new_trans = block.add_transactions();

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
    output_revevied->mutable_value()->set_value(
        how_ncc);  // std::to_string(how_ncc));

    neuro::messages::Output *output_sender = new_trans->add_outputs();
    output_sender->mutable_address()->CopyFrom(
        sender.outputs(num_output).address());
    output_sender->mutable_value()->set_value(
        total_ncc - how_ncc);  // std::to_string(total_ncc-how_ncc));

    new_trans->mutable_fees()->set_value(0);
    messages::hash_transaction(new_trans);

    /*
    std::cout << " --- AAA --- " << std::endl;
    Buffer buf;
    messages::to_buffer(*new_trans, &buf);
    messages::Hasher newit(buf);
    new_trans->mutable_id()->CopyFrom(newit);*/
  }

  neuro::Buffer buffake("123456");  // #1 fix require id in header of block
  neuro::messages::Hasher fake_id(buffake);
  header->mutable_id()->CopyFrom(fake_id);

  neuro::Buffer bufid;
  neuro::messages::to_buffer(block, &bufid);

  neuro::messages::Hasher blockid(bufid);
  header->mutable_id()->CopyFrom(blockid);

  return true;
}

bool genblock_from_last_db_block(
    messages::Block &block, std::shared_ptr<ledger::Ledger> ledger,
    const uint64_t seed, const int32_t new_height,
    std::optional<neuro::messages::KeyPub> author = std::nullopt,
    const int32_t last_height = 0, const int max_trx = 20,
    const int max_trail = 5) {
  int32_t height = last_height;
  if (height == 0) {
    height = ledger->height();
  }
  neuro::messages::Block last_block;
  if (!ledger->get_block(height, &last_block)) {
    if (!ledger->fork_get_block(height, &last_block)) {
      throw std::runtime_error({"Not found block - " + std::to_string(height)});
      return false;
    }
  }

  if (last_block.header().height() != height) {
    throw std::runtime_error({"Not found block 2 - " + std::to_string(height) +
                              " - " +
                              std::to_string(last_block.header().height())});
  }

  return genblock_from_block(block, last_block, seed, new_height, author,
                             max_trx, max_trail);
}

void create_first_blocks(const int nb, std::shared_ptr<ledger::Ledger> ledger) {
  for (int i = 1; i < nb; ++i) {
    messages::Block block;
    genblock_from_last_db_block(block, ledger, 1, i);

    ledger->push_block(block);
  }
}

void create_fork_blocks(const int nb, std::shared_ptr<ledger::Ledger> ledger) {
  messages::Block last_block, block;
  ledger->get_block(ledger->height(), &last_block);
  for (int i = 0; i < nb; ++i) {
    genblock_from_block(block, last_block, 1, last_block.header().height() + 1);
    ledger->fork_add_block(block);
    last_block.CopyFrom(block);
    block.Clear();
  }
}

}  // namespace genblock
}  // namespace tooling
}  // namespace neuro

#endif
