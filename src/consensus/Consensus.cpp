#include <assert.h>
#include <future>
#include <random>
#include <thread>

#include "common/Buffer.hpp"
#include "common/logger.hpp"
#include "consensus/Consensus.hpp"

namespace neuro {

namespace consensus {

Consensus::Consensus(std::shared_ptr<ledger::Ledger> ledger,
                     const std::vector<crypto::Ecc> &keys,
                     const PublishBlock &publish_block,
                     const VerifiedBlock &verified_block,
                     bool start_threads)
    : _ledger(ledger),
      _keys(keys),
      _publish_block(publish_block),
      _verified_block(verified_block) {
  init(start_threads);
}

Consensus::Consensus(std::shared_ptr<ledger::Ledger> ledger,
                     const std::vector<crypto::Ecc> &keys,
                     const std::optional<Config> &config,
                     const PublishBlock &publish_block,
                     const VerifiedBlock &verified_block, bool start_threads)
    : _config(config.value_or(Config())),
      _ledger(ledger),
      _keys(keys),
      _publish_block(publish_block),
      _verified_block(verified_block) {
  init(start_threads);
}

Consensus::Consensus(std::shared_ptr<ledger::Ledger> ledger,
                     const std::vector<crypto::Ecc> &keys, const Config &config,
                     const PublishBlock &publish_block,
                     const VerifiedBlock &verified_block, bool start_threads)
    : _config(config),
      _ledger(ledger),
      _keys(keys),
      _publish_block(publish_block),
      _verified_block(verified_block) {
  init(start_threads);
}

bool Consensus::check_outputs(
    const messages::Transaction &transaction) {
  messages::NCCValue total_received = 0;
  for (const auto &input : transaction.inputs()) {
    total_received += input.value().value();
  }

  messages::NCCValue total_spent = 0;
  for (const auto &output : transaction.outputs()) {
    total_spent += output.value().value();
    if(!output.has_key_pub() || !(output.key_pub().has_raw_data() || output.key_pub().has_hex_data())) {
      return false;
    }
  }

  if (transaction.has_fees()) {
    total_spent += transaction.fees().value();
  }

  bool result = total_spent == total_received;
  if (!result) {
    LOG_INFO << "Failed check_output for transaction "
             << transaction.id()
             << " the total amount available in the inputs " << total_received
             << " does not match the total amount spent " << total_spent;
  }
  return result;
}

bool Consensus::check_inputs(
    const messages::Transaction &transaction,
    const messages::TaggedBlock &tip) const {
  // Check that the sender have sufficient funds. There is no need to check it
  // when inserting a block because then the balances are checked in bulks. This
  // is used for cleaning up the transaction pool.
  for (const auto &input : transaction.inputs()) {
    if (_ledger->get_balance(input.key_pub(), tip).value().value() <
        input.value().value()) {
      LOG_DEBUG << "In transaction " << transaction.id()
                << " input " << input.key_pub()
                << " has insufficient funds at block "
                << tip.block().header().id();
      return false;
    }
  }
  return true;
}

bool Consensus::check_signatures(
    const messages::Transaction &transaction) const {
  bool result = crypto::verify(transaction);
  if (!result) {
    LOG_INFO << "Failed check_signatures for transaction "
             << transaction.id();
  }
  return result;
}

bool Consensus::check_id(const messages::TaggedTransaction &tagged_transaction,
                         const messages::TaggedBlock &tip) const {
  auto transaction = messages::Transaction(tagged_transaction.transaction());
  messages::set_transaction_hash(&transaction);
  if (!tagged_transaction.transaction().has_id()) {
    LOG_INFO << "Failed check_id the transaction has no id field";
    return false;
  }
  if (transaction.id() != tagged_transaction.transaction().id()) {
    LOG_INFO << "Failed check_id for transaction "
             << tagged_transaction.transaction().id();
    return false;
  }

  bool include_transaction_pool = !tagged_transaction.has_block_id();
  auto transactions = _ledger->get_transactions(transaction.id(), tip,
                                                include_transaction_pool);
  if (transactions.size() > 1) {
    LOG_INFO << "Failed check_id a transaction with the same id already exists";
    return false;
  }
  return true;
}

bool Consensus::check_double_inputs(
    const messages::TaggedTransaction &tagged_transaction) const {
  std::unordered_set<messages::_KeyPub> key_pubs;
  messages::_KeyPub key_pub;
  for (const auto &input : tagged_transaction.transaction().inputs()) {
    // We only want to compare the id and the output_id
    key_pub.CopyFrom(input.key_pub());
    if (key_pubs.count(key_pub) > 0) {
      LOG_INFO << "Failed check_double_inputs for transaction "
               << tagged_transaction.transaction().id();
      return false;
    }
    key_pubs.insert(key_pub);
  }
  return true;
}

bool Consensus::check_coinbase(
    const messages::TaggedTransaction &tagged_transaction,
    const messages::TaggedBlock &tip) const {
  messages::TaggedBlock tagged_block;
  if (!tagged_transaction.has_block_id() ||
      tagged_transaction.block_id() != tip.block().header().id()) {
    LOG_INFO << "Invalid coinbase for transaction "
             << tagged_transaction.transaction().id();
    return false;
  }

  messages::NCCValue total_fees = 0;
  for (const auto &transaction : tagged_block.block().transactions()) {
    if (transaction.has_fees()) {
      total_fees += transaction.fees().value();
    }
  }

  // Check total spent
  if (tagged_block.block().header().height() != 0) {
    messages::NCCValue to_spend =
        block_reward(tagged_block.block().header().height(), total_fees)
            .value();
    messages::NCCValue total_spent = 0;
    for (const auto &output : tagged_transaction.transaction().outputs()) {
      total_spent += output.value().value();
    }
    if (to_spend != total_spent) {
      LOG_INFO << "Failed check_coinbase for transaction "
               << tagged_transaction.transaction().id()
               << " expected coinbase was " << to_spend << " and got "
               << total_spent;

      return false;
    }
  }

  // Let's be strict here, a coinbase should not have any input or fee
  bool result = tagged_transaction.transaction().inputs_size() == 0 &&
                !tagged_transaction.transaction().has_fees();
  if (!result) {
    LOG_INFO << "Failed check_coinbase for transaction "
             << tagged_transaction.transaction().id();
  }
  return result;
}

messages::NCCAmount Consensus::block_reward(
    const messages::BlockHeight height, const messages::NCCValue fees) const {
  // TODO use some sort of formula here
  return messages::NCCAmount(_config.block_reward.value() + fees);
}

bool Consensus::check_block_id(
    const messages::TaggedBlock &tagged_block) const {
  auto mutable_tagged_block = tagged_block;
  auto block = mutable_tagged_block.mutable_block();
  const auto block_id = block->header().id();
  messages::set_block_hash(block);

  if (!block->header().has_id()) {
    LOG_INFO << "Failed check_block_id the block is missing the id field";
    return false;
  }

  if (block->header().id() != block_id) {
    LOG_INFO << "Failed check_block_id the block id is " << block_id
             << " but rehashing gave " << block->header().id();
    return false;
  }
  return true;
}

bool Consensus::is_unexpired(const messages::Transaction &transaction,
                             const messages::Block &block,
                             const messages::TaggedBlock &tip) const {
  messages::TaggedBlock last_seen_block;
  bool include_transactions = false;
  if (!_ledger->get_block(transaction.last_seen_block_id(), &last_seen_block,
                          include_transactions)) {
    LOG_INFO << "Failed to get the last_seen_block with id "
             << transaction.last_seen_block_id()
             << " when checking if transaction " << transaction.id()
             << " is expired in block at height " << block.header().height();
    return false;
  }
  if (!_ledger->is_ancestor(last_seen_block.branch_path(), tip.branch_path())) {
    LOG_INFO << "The transaction is invalid because the last_seen_block_id "
             << transaction.last_seen_block_id()
             << " is not in the correct branch when checking if transaction "
             << transaction.id() << " is expired in block at height "
             << block.header().height();
    return false;
  }

  int32_t expires = transaction.has_expires()
                        ? transaction.expires()
                        : _config.default_transaction_expires;
  if (block.header().height() - last_seen_block.block().header().height() >
      expires) {
    if (!block.header().has_id()) {
      LOG_INFO << "Transaction " << transaction.id() << " is expired";
      return false;
    }
    LOG_INFO << "Transaction " << transaction.id() << " is expired "
             << " in block at height " << block.header().height();
    return false;
  }
  return true;
}

bool Consensus::is_block_transaction_valid(
    const messages::TaggedTransaction &tagged_transaction,
    const messages::Block &block, const messages::TaggedBlock &tip) const {
  return is_valid(tagged_transaction, tip) &&
         is_unexpired(tagged_transaction.transaction(), block, tip);
}

bool Consensus::check_block_transactions(
    const messages::TaggedBlock &tagged_block) const {
  const auto &block = tagged_block.block();
  if (!block.has_coinbase()) {
    LOG_INFO << "Failed check_block_transactions for block "
             << block.header().id();
    return false;
  }

  messages::TaggedTransaction tagged_coinbase;
  tagged_coinbase.set_is_coinbase(true);
  tagged_coinbase.mutable_block_id()->CopyFrom(block.header().id());
  tagged_coinbase.mutable_transaction()->CopyFrom(block.coinbase());
  if (!is_block_transaction_valid(tagged_coinbase, block, tagged_block)) {
    LOG_INFO << "Failed check_block_transactions for block "
             << block.header().id();
    return false;
  }

  for (const auto transaction : block.transactions()) {
    messages::TaggedTransaction tagged_transaction;
    tagged_transaction.set_is_coinbase(false);
    tagged_transaction.mutable_block_id()->CopyFrom(block.header().id());
    tagged_transaction.mutable_transaction()->CopyFrom(transaction);
    if (!is_block_transaction_valid(tagged_transaction, block, tagged_block)) {
      LOG_INFO << "Failed check_block_transactions for block "
               << block.header().id();
      return false;
    }
  }

  return true;
}

bool Consensus::check_block_size(
    const messages::TaggedBlock &tagged_block) const {
  const auto &block = tagged_block.block();
  Buffer buffer;
  messages::to_buffer(block, &buffer);
  bool result = buffer.size() < _config.max_block_size;
  if (!result) {
    LOG_INFO << "Failed check_block_size for block " << block.header().id();
  }
  return result;
}

bool Consensus::check_transactions_order(const messages::Block &block) const {
  std::vector<std::string> transaction_ids;
  for (const auto &transaction : block.transactions()) {
    transaction_ids.push_back(messages::to_json(transaction.id()));
  }

  std::vector<std::string> orig_transaction_ids{transaction_ids};
  std::sort(transaction_ids.begin(), transaction_ids.end());

  bool result = orig_transaction_ids == transaction_ids;
  if (!result) {
    LOG_INFO << "Failed check_transactions_order for block "
             << block.header().id();
  }
  return result;
}

bool Consensus::check_transactions_order(
    const messages::TaggedBlock &tagged_block) const {
  const auto &block = tagged_block.block();
  return check_transactions_order(block);
}

bool Consensus::check_block_height(
    const messages::TaggedBlock &tagged_block) const {
  const auto &block = tagged_block.block();
  messages::Block previous;
  if (!_ledger->get_block(block.header().previous_block_hash(), &previous,
                          false)) {
    LOG_INFO << "Failed check_block_height for block " << block.header().id()
             << " failed to get the previous_block";
    return false;
  }

  bool result = block.header().height() > previous.header().height();
  if (!result) {
    LOG_INFO << "Failed check_block_height for block " << block.header().id()
             << " height " << block.header().height()
             << " is lower than the height of the previous block "
             << previous.header().height();
  }
  return result;
}

bool Consensus::check_block_timestamp(
    const messages::TaggedBlock &tagged_block) const {
  const auto &block = tagged_block.block();
  messages::Block block0;
  _ledger->get_block(0, &block0);
  int64_t expected_timestamp = block0.header().timestamp().data() +
                               block.header().height() * _config.block_period;
  int64_t real_timestamp = tagged_block.block().header().timestamp().data();
  if (!block.header().has_timestamp()) {
    LOG_INFO << "Failed check_block_timestamp for block " << block.header().id()
             << " it is missing the timestamp field";
    return false;
  }
  if (real_timestamp < expected_timestamp) {
    LOG_INFO << "Failed check_block_timestamp for block " << block.header().id()
             << " it is too early" << std::endl
             << "Real timestamp " << real_timestamp << std::endl
             << "Expected timestamp " << expected_timestamp << std::endl;
    return false;
  }
  if (real_timestamp > expected_timestamp + _config.block_period) {
    LOG_INFO << "Failed check_block_timestamp for block " << block.header().id()
             << " it is too late" << std::endl
             << "Real timestamp " << real_timestamp << std::endl
             << "Expected timestamp " << expected_timestamp << std::endl;
    return false;
  }
  return true;
}

bool Consensus::check_block_author(
    const messages::TaggedBlock &tagged_block) const {
  messages::TaggedBlock previous;
  messages::Assembly previous_assembly, previous_previous_assembly;
  const auto &block = tagged_block.block();

  if (!_ledger->get_block(tagged_block.block().header().previous_block_hash(),
                          &previous, false)) {
    LOG_INFO << "Failed check_block_author because it failed to get the "
                "previous block "
             << tagged_block.block().header().previous_block_hash();
    return false;
  }
  if (!previous.has_previous_assembly_id()) {
    LOG_INFO << "Failed check_block_author for block " << block.header().id()
             << " the previous block is missing field previous_assembly_id";
    return false;
  }
  if (!_ledger->get_assembly(previous.previous_assembly_id(),
                             &previous_assembly)) {
    LOG_INFO << "Failed check_block_author for block " << block.header().id()
             << " failed to get previous assembly "
             << tagged_block.previous_assembly_id();
    return false;
  }
  if (!_ledger->get_assembly(previous_assembly.previous_assembly_id(),
                             &previous_previous_assembly)) {
    LOG_INFO << "Failed check_block_author for block " << block.header().id()
             << " failed to get previous previous assembly "
             << previous_assembly.previous_assembly_id();
    return false;
  }
  if (is_new_assembly(tagged_block, previous)) {
    previous_previous_assembly = previous_assembly;
  }

  messages::_KeyPub key_pub;
  if (!get_block_writer(previous_previous_assembly, block.header().height(),
                        &key_pub)) {
    LOG_INFO << "Failed check_block_author for block " << block.header().id()
             << " could not find the block writer";
    return false;
  }

  const auto author_key_pub =
      messages::_KeyPub(block.header().author().key_pub());
  if (key_pub != author_key_pub) {
    LOG_INFO << "Failed check_block_author for block " << block.header().id()
             << " the author key_pub " << author_key_pub
             << " does not match the required block writer " << key_pub;
    return false;
  }

  if (!crypto::verify(block)) {
    LOG_INFO << "Failed check_block_author for block " << block.header().id()
             << " the signature is wrong";
    return false;
  }
  return true;
}

bool Consensus::check_block_denunciations(
    const messages::TaggedBlock &tagged_block) const {
  for (const auto denunciation : tagged_block.block().denunciations()) {
    // Check that we do have a block written by the same author at the same
    // height The issue is that I do need to get all the blocks at the given
    // height
    // We should also check that it has not been already denunced in this branch
    bool include_transactions = false;
    messages::TaggedBlock double_mining_block;
    if (!_ledger->get_block(denunciation.block_height(),
                            tagged_block.branch_path(), &double_mining_block,
                            include_transactions)) {
      LOG_INFO << "Failed check_block_denunciations for block "
               << tagged_block.block().header().id() << " invalid denunciation "
               << denunciation << " there was no double mining";
      return false;
    }
    if (_ledger->denunciation_exists(denunciation,
                                     tagged_block.block().header().height() - 1,
                                     tagged_block.branch_path())) {
      LOG_INFO << "Failed check_block_denunciations for block "
               << tagged_block.block().header().id() << " invalid denunciation "
               << denunciation << " the denunciation is a duplicate";
      return false;
    }
  }
  return true;
}

messages::BlockScore Consensus::get_block_score(
    const messages::TaggedBlock &tagged_block) const {
  messages::TaggedBlock previous;
  if (!_ledger->get_block(tagged_block.block().header().previous_block_hash(),
                          &previous) ||
      !previous.has_score()) {
    throw std::runtime_error(
        "Trying to score block " +
        messages::to_json(tagged_block.block().header().id()) +
        " when the previous block does not have a score");
  }
  return previous.score() + 1;
}

void Consensus::process_blocks() {
  std::lock_guard<std::mutex> lock(_process_blocks_mutex);

  // Use wait_for() with zero milliseconds to check thread status.
  if (_process_blocks_future.valid() &&
      _process_blocks_future.wait_for(0ms) != std::future_status::ready) {
    return;
  }

  _process_blocks_future = std::async(std::launch::async, [this]() {
    verify_blocks();
    _ledger->update_main_branch();
  });
}

bool Consensus::verify_blocks() {
  auto tagged_blocks = _ledger->get_unverified_blocks();
  for (auto tagged_block : tagged_blocks) {
    _ledger->fill_block_transactions(tagged_block.mutable_block());
    messages::TaggedBlock previous;
    if (!_ledger->get_block(tagged_block.block().header().previous_block_hash(),
                            &previous, false)) {
      // Something is wrong here
      throw std::runtime_error(
          "Could not find the previous block of an unverified block " +
          messages::to_json(tagged_block.block().header().id()));
    }
    if (previous.branch() == messages::INVALID) {
      if (!_ledger->set_branch_invalid(tagged_block.block().header().id())) {
        throw std::runtime_error("Failed to mark a block as invalid");
      } else {
        LOG_WARNING << "Invalid block in verify_blocks "
                    << tagged_block.block().header().id() << " at height "
                    << tagged_block.block().header().height();

        // The list of unverified blocks should have changed
        verify_blocks();
        return false;
      }
    }
    if (previous.branch() == messages::UNVERIFIED) {
      // Probably because the assembly computations are not finished
      continue;
    }

    messages::AssemblyID assembly_id;
    messages::Assembly previous_assembly, previous_previous_assembly;
    if (is_new_assembly(tagged_block, previous)) {
      const messages::AssemblyHeight height =
          previous.block().header().height() / _config.blocks_per_assembly;
      _ledger->add_assembly(previous, height);
      assembly_id = previous.block().header().id();
      if (!_ledger->get_assembly(previous.previous_assembly_id(),
                                 &previous_previous_assembly)) {
        LOG_ERROR << "Failed to get assembly "
                  << previous_assembly.previous_assembly_id();
        return false;
      }
    } else {
      if (!previous.has_previous_assembly_id()) {
        throw std::runtime_error(
            "A verified tagged_block is missing a previous_assembly_id " +
            messages::to_json(previous.block().header().id()));
      }
      assembly_id = previous.previous_assembly_id();
      if (!_ledger->get_assembly(previous.previous_assembly_id(),
                                 &previous_assembly)) {
        LOG_ERROR << "Failed to get assembly "
                  << previous.previous_assembly_id();
        return false;
      }
      if (!_ledger->get_assembly(previous_assembly.previous_assembly_id(),
                                 &previous_previous_assembly)) {
        LOG_ERROR << "Failed to get assembly "
                  << previous_assembly.previous_assembly_id();
        return false;
      }
    }

    if (!previous_previous_assembly.finished_computation()) {
      LOG_DEBUG << "Cannot validate block "
                << tagged_block.block().header().id()
                << " because the assembly computation is not finished.";
      continue;
    }

    if (_ledger->add_balances(&tagged_block, _config.blocks_per_assembly) &&
        is_valid(tagged_block)) {
      bool is_verified = _ledger->set_block_verified(
          tagged_block.block().header().id(), get_block_score(tagged_block),
          assembly_id);
      if (is_verified) {
        _verified_block(tagged_block.block());
      }
    } else if (!_ledger->set_branch_invalid(
                   tagged_block.block().header().id())) {
      throw std::runtime_error("Failed to mark a block as invalid");
    } else {
      // The list of unverified blocks should have changed
      verify_blocks();
      return false;
    }
  }

  return true;
}

bool Consensus::is_new_assembly(const messages::TaggedBlock &tagged_block,
                                const messages::TaggedBlock &previous) const {
  auto block_assembly_height =
      tagged_block.block().header().height() / _config.blocks_per_assembly;
  auto previous_assembly_height =
      previous.block().header().height() / _config.blocks_per_assembly;
  return block_assembly_height != previous_assembly_height;
}

Config Consensus::config() const { return _config; }

void Consensus::init(bool start_threads) {
  for (const auto &key : _keys) {
    _key_pubs.emplace_back(key.key_pub());
  }

  _is_compute_pii_stopped = false;
  _is_update_heights_stopped = false;
  _is_miner_stopped = false;
  if (start_threads) {
    start_compute_pii_thread();
    start_update_heights_thread();
    start_miner_thread();
  }
}

Consensus::~Consensus() {
  LOG_DEBUG << this << " Entered consensus destructor";
  _is_compute_pii_stopped = true;
  _is_update_heights_stopped = true;
  _is_miner_stopped = true;
  _is_stopped_cv.notify_all();
  if (_compute_pii_thread.joinable()) {
    _compute_pii_thread.join();
  }
  if (_update_heights_thread.joinable()) {
    _update_heights_thread.join();
  }
  if (_miner_thread.joinable()) {
    _miner_thread.join();
  }
  if (_process_blocks_future.valid()) {
    _process_blocks_future.wait();
  }
  LOG_DEBUG << this << " Leaving consensus destructor";
}

bool Consensus::is_valid(const messages::TaggedTransaction &tagged_transaction,
                         const messages::TaggedBlock &tip) const {
  if (tagged_transaction.is_coinbase()) {
    return check_id(tagged_transaction, tip) &&
           check_coinbase(tagged_transaction, tip);
  }
  return check_id(tagged_transaction, tip) &&
         check_signatures(tagged_transaction.transaction()) &&
         check_double_inputs(tagged_transaction) &&
         check_outputs(tagged_transaction.transaction());
}

bool Consensus::is_valid(const messages::TaggedBlock &tagged_block) const {
  // This method should be only be called after the block has been inserted
  bool result =
      check_block_id(tagged_block) && check_block_transactions(tagged_block) &&
      check_block_size(tagged_block) &&
      check_transactions_order(tagged_block) &&
      check_block_height(tagged_block) && check_block_timestamp(tagged_block) &&
      check_block_author(tagged_block) &&
      check_block_denunciations(tagged_block);
  if (!result) {
    LOG_WARNING << "Invalid block " << tagged_block.block().header().id()
                << " at height " << tagged_block.block().header().height();
  }
  return result;
}

bool Consensus::add_transaction(const messages::Transaction &transaction) {
  messages::TaggedTransaction tagged_transaction;
  tagged_transaction.set_is_coinbase(false);
  tagged_transaction.mutable_transaction()->CopyFrom(transaction);
  const auto &tip = _ledger->get_main_branch_tip();
  return is_valid(tagged_transaction, tip) &&
         check_inputs(tagged_transaction.transaction(), tip) &&
         _ledger->add_to_transaction_pool(transaction);
}

bool Consensus::add_double_mining(const messages::Block &block) {
  bool include_transactions = false;
  auto blocks = _ledger->get_blocks(block.header().height(),
                                    block.header().author().key_pub(),
                                    include_transactions);
  if (blocks.size() > 1) {
    _ledger->add_double_mining(blocks);
  }
  return true;
}

bool Consensus::add_block(const messages::Block &block, bool async) {
  // Check the transactions order before inserting because the order is lost at
  // insertion
  if (!check_transactions_order(block)) {
    LOG_WARNING << "failed to add block due to transaction order : " << block;
    return false;
  }
  if (!_ledger->insert_block(block)) {
    LOG_WARNING << "failed to add block due to insert_block : " << block;
    return false;
  }
  if (!add_double_mining(block)) {
    LOG_WARNING << "failed to add block due to double mining : " << block;
    return false;
  }
  if (async) {
    process_blocks();
  } else {
    if (!verify_blocks()) {
      LOG_WARNING << "failed to add block due to verify_block : " << block;
      return false;
    }
    if (!_ledger->update_main_branch()) {
      LOG_WARNING << "failed to add block due to update_main_branch : "
                  << block;
      return false;
    }
  }
  return true;
}

bool Consensus::add_block(const messages::Block &block) {
  return add_block(block, false);
}

bool Consensus::add_block_async(const messages::Block &block) {
  return add_block(block, true);
}

void Consensus::start_compute_pii_thread() {
  if (!_compute_pii_thread.joinable()) {
    _compute_pii_thread = std::thread([this]() {
      while (!_is_compute_pii_stopped) {
        std::vector<messages::Assembly> assemblies;
        _ledger->get_assemblies_to_compute(&assemblies);
        if (!assemblies.empty()) {
          LOG_DEBUG << "Starting assembly computations " << assemblies[0].id();
          compute_assembly_pii(assemblies[0]);
          LOG_DEBUG << "Finished assembly computations " << assemblies[0].id();
          cleanup_transaction_pool();
        } else {
          std::unique_lock cv_lock(_is_stopped_mutex);
          _is_stopped_cv.wait_for(cv_lock, _config.compute_pii_sleep,
                                  [&]() { return _is_compute_pii_stopped; });
        }
      }
    });
  }
}

bool Consensus::compute_assembly_pii(const messages::Assembly &assembly) {
  auto pii = Pii(_ledger, _config);
  auto integrities = Integrities(_config);
  messages::TaggedBlock tagged_block;
  if (!_ledger->get_block(assembly.id(), &tagged_block)) {
    LOG_WARNING << "During Pii computation missing block with id assembly_id "
                << assembly.id();
    return false;
  }
  uint32_t seed = 0;
  const auto branch_path = tagged_block.branch_path();

  while (tagged_block.block().header().id() !=
         assembly.previous_assembly_id()) {
    if (!pii.add_block(tagged_block)) {
      LOG_WARNING << "Failed to compute the Pii of block "
                  << tagged_block.block().header().id();
      return false;
    }
    integrities.add_block(tagged_block);
    auto block_id = tagged_block.block().header().id().data();
    seed <<= 1;
    seed += block_id.at(block_id.size() - 1) % 2;
    if (tagged_block.block().header().height() == 0) {
      break;
    }
    if (!_ledger->get_block(tagged_block.block().header().previous_block_hash(),
                            &tagged_block)) {
      LOG_WARNING << "During Pii computation missing block "
                  << tagged_block.block().header().previous_block_hash();
      return false;
    }
  }

  if (!_ledger->set_seed(assembly.id(), seed)) {
    LOG_WARNING << "During Pii computation failed to set seed for assembly "
                << assembly.id();
    return false;
  }

  // Add the integrity before the pii because it is used in the pii computations
  for (const auto &[key_pub, score] : integrities.scores()) {
    _ledger->add_integrity(key_pub, assembly.id(), assembly.height(),
                           branch_path, score);
  }

  auto piis = pii.get_key_pubs_pii(assembly.height(), branch_path);
  if (piis.empty()) {
    LOG_INFO << "There were no transactions during the assembly "
             << assembly.id()
             << " we will therefore use the piis of the previous assembly";
    if (!_ledger->get_assembly_piis(assembly.previous_assembly_id(), &piis)) {
      LOG_WARNING << "Failed to get the previous assembly piis during pii "
                     "computation of assembly "
                  << assembly.id();
      return false;
    }
  }
  for (size_t i = 0; i < piis.size(); i++) {
    auto pii = piis[i];
    pii.mutable_assembly_id()->CopyFrom(assembly.id());
    pii.set_rank(i);
    _ledger->set_pii(pii);
  }

  if (!_ledger->set_nb_key_pubs(assembly.id(), piis.size())) {
    LOG_WARNING
        << "During Pii computation failed to set nb_key_pubs for assembly "
        << assembly.id();
    return false;
  }
  if (!_ledger->set_finished_computation(assembly.id())) {
    LOG_WARNING << "During Pii computation failed to set "
                   "finished_computation for assembly "
                << assembly.id();
    return false;
  }
  return true;
}

bool Consensus::get_block_writer(const messages::Assembly &assembly,
                                 const messages::BlockHeight &height,
                                 messages::_KeyPub *key_pub) const {
  if (!assembly.has_seed()) {
    LOG_WARNING << "Failed get_block_writer assembly " << assembly.id()
                << " is missing the seed";
    return false;
  }

  if (!assembly.has_nb_key_pubs()) {
    LOG_INFO << "Failed get_block_writer assembly " << assembly.id()
             << " is missing the number of key_pubs";
    return false;
  }
  std::mt19937 rng;
  rng.seed(assembly.seed() + height);
  auto dist = std::uniform_int_distribution<std::mt19937::result_type>(
      0,
      std::min(assembly.nb_key_pubs(), (int32_t)_config.members_per_assembly) -
          1);
  auto writer_rank = dist(rng);
  if (!_ledger->get_block_writer(assembly.id(), writer_rank, key_pub)) {
    LOG_WARNING << "Could not find block writer of rank " << writer_rank
                << " in assembly " << assembly.id();
    return false;
  }

  return true;
}

messages::BlockHeight Consensus::get_current_height() const {
  messages::Block block0;
  _ledger->get_block(0, &block0);
  return (std::time(nullptr) - block0.header().timestamp().data()) /
         _config.block_period;
}

messages::BlockHeight Consensus::get_current_assembly_height() const {
  return get_current_height() / _config.blocks_per_assembly;
}

bool Consensus::get_heights_to_write(
    const std::vector<messages::_KeyPub> &key_pubs,
    std::vector<std::pair<messages::BlockHeight, KeyPubIndex>> *heights) const {
  const messages::TaggedBlock last_block = _ledger->get_main_branch_tip();
  const auto last_block_height = last_block.block().header().height();
  messages::Assembly previous_assembly, previous_previous_assembly;

  if (!_ledger->get_assembly(last_block.previous_assembly_id(),
                             &previous_assembly)) {
    LOG_WARNING
        << "Could not find the previous assembly in get_heights_to_write";
    return false;
  }
  if (!_ledger->get_assembly(previous_assembly.previous_assembly_id(),
                             &previous_previous_assembly)) {
    LOG_WARNING << "Could not find the previous previous assembly in "
                   "get_heights_to_write";
    return false;
  }
  if (!previous_previous_assembly.finished_computation()) {
    LOG_WARNING << "The computation of the assembly "
                << previous_previous_assembly.id() << " is not finished.";
    return false;
  }
  std::unordered_map<messages::_KeyPub, KeyPubIndex> key_pubs_map;
  for (uint32_t i = 0; i < key_pubs.size(); i++) {
    key_pubs_map[key_pubs[i]] = i;
  }

  // I never want anyone to mine the block 0
  const int32_t current_height = std::max(get_current_height(), 1);
  const int32_t last_block_assembly_height =
      last_block_height / _config.blocks_per_assembly;

  int32_t first_height = last_block_height + 1;
  int32_t last_height =
      (last_block_assembly_height + 1) * _config.blocks_per_assembly - 1;
  for (int32_t i = std::max(current_height, first_height); i <= last_height;
       i++) {
    messages::_KeyPub writer;
    if (!get_block_writer(previous_previous_assembly, i, &writer)) {
      LOG_WARNING << "Did not manage to get the block writer for assembly "
                  << previous_previous_assembly.id() << " at height " << i;
    } else if (key_pubs_map.count(writer) > 0) {
      heights->push_back({i, key_pubs_map[writer]});
    }
  }

  if (previous_assembly.finished_computation()) {
    // If an assembly does not have any block then the assembly is repeated
    // until it does get a block
    int32_t current_assembly_height =
        current_height / _config.blocks_per_assembly;
    int32_t assembly_height_to_mine =
        std::max(current_assembly_height, last_block_assembly_height + 1);
    first_height = assembly_height_to_mine * _config.blocks_per_assembly;
    last_height =
        (assembly_height_to_mine + 1) * _config.blocks_per_assembly - 1;

    for (auto i = std::max(current_height, first_height); i <= last_height;
         i++) {
      messages::_KeyPub writer;
      if (!get_block_writer(previous_assembly, i, &writer)) {
        LOG_WARNING << "Did not manage to get the block writer for assembly "
                    << previous_assembly.id() << " at height " << i;
      } else if (key_pubs_map.count(writer) > 0) {
        heights->push_back({i, key_pubs_map[writer]});
      }
    }
  }

  return true;
}

bool Consensus::cleanup_transactions(messages::Block *block) const {
  std::lock_guard lock(mpfr_mutex);  // TODO trax, is it really needed?
  messages::TaggedBlock previous;
  if (!_ledger->get_block(block->header().previous_block_hash(), &previous)) {
    LOG_INFO << "Failed to get the previous block with id "
             << block->header().previous_block_hash()
             << " in cleanup transactions";
    return false;
  }

  std::unordered_map<messages::_KeyPub, Double> balances;
  messages::Block accepted_transactions;

  for (const messages::Transaction &transaction : block->transactions()) {
    if (!is_unexpired(transaction, *block, previous)) {
      _ledger->delete_transaction(transaction.id());
      continue;
    }

    bool is_transaction_valid = true;
    for (const auto &input : transaction.inputs()) {
      auto &key_pub = input.key_pub();
      if (balances.count(key_pub) == 0) {
        balances[key_pub] =
            _ledger->get_balance(key_pub, previous).value().value();
      }
      balances[key_pub] -= input.value().value();
      if (balances[key_pub] < 0) {
        is_transaction_valid = false;
      }
    }

    if (!is_transaction_valid) {
      LOG_INFO << "Transaction " << transaction
               << " not included in the block because of insufficient funds";

      _ledger->delete_transaction(transaction.id());

      // Reverse balance change
      for (const auto &input : transaction.inputs()) {
        balances[input.key_pub()] += input.value().value();
      }
      continue;
    }

    for (const auto &output : transaction.outputs()) {
      auto &key_pub = output.key_pub();
      if (balances.count(key_pub) == 0) {
        balances[key_pub] =
            _ledger->get_balance(key_pub, previous).value().value();
      }
      balances[key_pub] += output.value().value();
    }
    accepted_transactions.add_transactions()->CopyFrom(transaction);
  }

  block->mutable_transactions()->Swap(
      accepted_transactions.mutable_transactions());

  return true;
}

bool Consensus::build_block(const crypto::Ecc &keys,
                            const messages::BlockHeight &height,
                            messages::Block *block) const {
  messages::TaggedBlock last_block = _ledger->get_main_branch_tip();
  auto &last_block_header = last_block.block().header();
  if (last_block_header.height() >= height) {
    LOG_WARNING << "Trying to mine block with height " << height
                << " while previous block has height "
                << last_block_header.height();
    return false;
  }
  auto header = block->mutable_header();
  header->set_height(height);
  header->mutable_previous_block_hash()->CopyFrom(last_block_header.id());
  header->mutable_timestamp()->set_data(std::time(nullptr));

  _ledger->get_transaction_pool(block, _config.max_block_size,
                                _config.max_transaction_per_block);
  if (!cleanup_transactions(block)) {
    LOG_WARNING
        << "Failed to cleanup_transactions while mining block with height "
        << block->header().height();
    return false;
  }

  messages::NCCValue total_fees = 0;
  for (const auto &transaction : block->transactions()) {
    if (transaction.has_fees()) {
      total_fees += transaction.fees().value();
    }
  }

  // Block reward
  tooling::blockgen::coinbase(
      {keys.key_pub()}, block_reward(height, total_fees),
      block->mutable_coinbase(), last_block.block().header().id());

  // Check if the coinbase needs a longer expires
  if (!is_unexpired(block->coinbase(), *block, last_block)) {
    block->mutable_coinbase()->set_expires(
        block->header().height() - last_block.block().header().height());
    messages::set_transaction_hash(block->mutable_coinbase());
  }

  _ledger->add_denunciations(block, last_block.branch_path());

  messages::sort_transactions(block);
  messages::set_block_hash(block);
  crypto::sign(keys, block);

  return true;
}

void Consensus::start_update_heights_thread() {
  if (!_update_heights_thread.joinable()) {
    _update_heights_thread = std::thread([this]() {
      while (!_is_update_heights_stopped) {
        // Makes sure process blocks is called regularly
        process_blocks();

        update_heights_to_write();
        std::unique_lock cv_lock(_is_stopped_mutex);
        _is_stopped_cv.wait_for(cv_lock, _config.update_heights_sleep,
                                [&]() { return _is_update_heights_stopped; });
      }
    });
  }
}

void Consensus::update_heights_to_write() {
  messages::TaggedBlock tagged_block;
  bool include_transactions = false;
  if (!_ledger->get_last_block(&tagged_block, include_transactions)) {
    LOG_ERROR << "Failed to get the last block in update_heights_to_mine";
    return;
  }
  messages::Assembly assembly;
  if (!_ledger->get_assembly(tagged_block.previous_assembly_id(), &assembly)) {
    LOG_ERROR << "Failed to get the assembly with id "
              << tagged_block.previous_assembly_id()
              << " in update_heights_to_mine";
    return;
  }
  if (!assembly.finished_computation()) {
    return;
  }

  // If thet previous assembly has not changed there is no need to recompute the
  // heights
  auto current_assembly_height = get_current_assembly_height();
  if (_previous_assembly_id && _current_assembly_height &&
      _previous_assembly_id.value() == tagged_block.previous_assembly_id() &&
      _current_assembly_height == current_assembly_height) {
    return;
  }

  std::vector<std::pair<messages::BlockHeight, KeyPubIndex>> heights;
  get_heights_to_write(_key_pubs, &heights);
  _heights_to_write_mutex.lock();
  _heights_to_write = heights;
  _heights_to_write_mutex.unlock();
  _previous_assembly_id = tagged_block.previous_assembly_id();
  _current_assembly_height = current_assembly_height;
}

void Consensus::start_miner_thread() {
  if (!_miner_thread.joinable()) {
    _miner_thread = std::thread([this]() { mine_blocks(); });
  }
}

bool Consensus::mine_block(const messages::Block &block0) {
  std::lock_guard<std::mutex> lock(_heights_to_write_mutex);
  if (_heights_to_write.empty()) {
    return false;
  }
  const auto [height, key_pub_index] = _heights_to_write.front();
  const auto block_start =
      block0.header().timestamp().data() + height * _config.block_period;
  const std::time_t block_end = block_start + _config.block_period;
  const std::time_t current_time = std::time(nullptr);

  if (current_time < block_start) {
    return false;
  }
  _heights_to_write.erase(_heights_to_write.begin());

  if (current_time >= block_end) {
    return false;
  }

  if (_last_mined_block_height == height) {
    LOG_INFO << "Attempt to mine a block that we have already mined at height "
             << height;
    return false;
  }

  messages::Block new_block;
  if (!build_block(_keys[key_pub_index], height, &new_block)) {
    return false;
  }

  // Check that the block author is correct
  messages::TaggedBlock tagged_block;
  tagged_block.mutable_block()->CopyFrom(new_block);
  if (!check_block_author(tagged_block)) {
    LOG_WARNING << "Failed to mine block because the block author is wrong";
    return false;
  }

  // Check that the newly created block isn't late
  if (new_block.header().timestamp().data() >= block_end) {
    return false;
  }

  const auto t0 = Timer::now();
  LOG_DEBUG << "running _publish_block";
  _publish_block(new_block);
  LOG_DEBUG << "_publish_block took " << (Timer::now() - t0).count() / 1E6
            << " ms";
  const auto t1 = Timer::now();

  // We don't add the block in an async way because we don't want to mine the
  // next block as a fork

  if(!add_block(new_block)) {
    LOG_ERROR << "Could not add mined block ";
    return false;
  }
  
  LOG_DEBUG << "add_block took " << (Timer::now() - t1).count() / 1E6 << " ms";

  _last_mined_block_height = height;

  LOG_INFO << "Mined block successfully with id " << new_block.header().id()
           << " with height " << new_block.header().height();
  return true;
}

void Consensus::mine_blocks() {
  messages::Block block0;
  if (!_ledger->get_block(0, &block0)) {
    throw std::runtime_error("Failed to get block0 in start_miner_thread");
  }
  while (!_is_miner_stopped) {
    if (!mine_block(block0)) {
      std::unique_lock cv_lock(_is_stopped_mutex);
      _is_stopped_cv.wait_for(cv_lock, _config.miner_sleep,
                              [&]() { return _is_miner_stopped; });
    }
  }
}

void Consensus::cleanup_transaction_pool() {
  const auto &tip = _ledger->get_main_branch_tip();
  for (const auto &tagged_transaction : _ledger->get_transaction_pool()) {
    if (!is_unexpired(tagged_transaction.transaction(), tip.block(), tip) ||
        !check_inputs(tagged_transaction.transaction(), tip)) {
      _ledger->delete_transaction(tagged_transaction.transaction().id());
    }
  }
}

}  // namespace consensus
}  // namespace neuro
