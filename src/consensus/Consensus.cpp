#include "consensus/Consensus.hpp"
#include <algorithm>
#include <chrono>
#include <functional>
#include <random>
#include <thread>
#include <unordered_set>
#include "tooling/blockgen.hpp"

namespace neuro {
namespace tooling {
namespace tests {
class RealtimeSimulator;
}
}  // namespace tooling

namespace consensus {

namespace tests {
class Consensus;
class RealtimeConsensus;
}  // namespace tests

Consensus::Consensus(std::shared_ptr<ledger::Ledger> ledger,
                     const std::vector<crypto::Ecc> &keys,
                     const PublishBlock &publish_block, bool start_threads)
    : _ledger(ledger), _keys(keys), _publish_block(publish_block) {
  init(start_threads);
}

Consensus::Consensus(std::shared_ptr<ledger::Ledger> ledger,
                     const std::vector<crypto::Ecc> &keys,
                     const std::optional<Config> &config,
                     const PublishBlock &publish_block, bool start_threads)
    : _config(config.value_or(Config())),
      _ledger(ledger),
      _keys(keys),
      _publish_block(publish_block) {
  init(start_threads);
}

Consensus::Consensus(std::shared_ptr<ledger::Ledger> ledger,
                     const std::vector<crypto::Ecc> &keys, const Config &config,
                     const PublishBlock &publish_block, bool start_threads)
    : _config(config),
      _ledger(ledger),
      _keys(keys),
      _publish_block(publish_block) {
  init(start_threads);
}

bool Consensus::check_inputs(
    const messages::TaggedTransaction &tagged_transaction) const {
  return true;
}

bool Consensus::check_outputs(
    const messages::TaggedTransaction tagged_transaction) const {
  messages::TaggedBlock tip;
  if (tagged_transaction.has_block_id()) {
    if (!_ledger->get_block(tagged_transaction.block_id(), &tip, false)) {
      LOG_INFO << "Failed check_output for transaction "
               << tagged_transaction.transaction().id();
      return false;
    };
  } else {
    tip = _ledger->get_main_branch_tip();
  }

  messages::NCCValue total_received = 0;
  for (const auto &input : tagged_transaction.transaction().inputs()) {
    total_received += input.value().value();
  }

  messages::NCCValue total_spent = 0;
  for (const auto &output : tagged_transaction.transaction().outputs()) {
    total_spent += output.value().value();
  }

  if (tagged_transaction.transaction().has_fees()) {
    total_spent += tagged_transaction.transaction().fees().value();
  }

  bool result = total_spent == total_received;
  if (!result) {
    LOG_INFO << "Failed check_output for transaction "
             << tagged_transaction.transaction().id()
             << " the total amount available in the inputs " << total_received
             << " does not match the total amount spent " << total_spent;
  }
  return result;
}

bool Consensus::check_signatures(
    const messages::TaggedTransaction &tagged_transaction) const {
  bool result = crypto::verify(tagged_transaction.transaction());
  if (!result) {
    LOG_INFO << "Failed check_signatures for transaction "
             << tagged_transaction.transaction().id();
  }
  return result;
}

bool Consensus::check_id(
    const messages::TaggedTransaction &tagged_transaction) const {
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

  // Check that there is no other transaction with the same id in the same
  // branch
  messages::TaggedBlock tip;
  if (tagged_transaction.has_block_id()) {
    if (!_ledger->get_block(tagged_transaction.block_id(), &tip, false)) {
      LOG_INFO << "Could not find transaction block for transaction "
               << tagged_transaction.transaction().id();
      return false;
    }
  } else {
    tip = _ledger->get_main_branch_tip();
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
    const messages::TaggedTransaction &tagged_transaction) const {
  messages::TaggedBlock tagged_block;
  if (!tagged_transaction.has_block_id() ||
      !_ledger->get_block(tagged_transaction.block_id(), &tagged_block)) {
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
  const auto author = block->header().author();
  messages::set_block_hash(block);

  if (!block->header().has_id()) {
    LOG_INFO << "Failed check_block_id the block is missing the id field";
    return false;
  }

  return block->header().id() == block_id;
}

bool Consensus::is_unexpired(const messages::Transaction &transaction,
                             const messages::Block &block) const {
  messages::Block last_seen_block;
  bool include_transactions = false;
  if (!_ledger->get_block(transaction.last_seen_block_id(), &last_seen_block,
                          include_transactions)) {
    LOG_INFO << "Failed to get the last_seen_block with id "
             << transaction.last_seen_block_id()
             << " when checking if transaction " << transaction.id()
             << " is expired in block " << block.header().id();
    return false;
  }
  int32_t expires = transaction.has_expires()
                        ? transaction.expires()
                        : _config.default_transaction_expires;
  if (block.header().height() - last_seen_block.header().height() > expires) {
    LOG_INFO << "Transaction " << transaction.id() << " is expired "
             << " in block " << block.header().id();
    return false;
  }
  return true;
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
  if (!is_valid(tagged_coinbase) && !is_unexpired(block.coinbase(), block)) {
    LOG_INFO << "Failed check_block_transactions for block "
             << block.header().id();
    return false;
  }

  for (const auto transaction : block.transactions()) {
    messages::TaggedTransaction tagged_transaction;
    tagged_transaction.set_is_coinbase(false);
    tagged_transaction.mutable_block_id()->CopyFrom(block.header().id());
    tagged_transaction.mutable_transaction()->CopyFrom(transaction);
    if (!is_valid(tagged_transaction) || !is_unexpired(transaction, block)) {
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

bool Consensus::check_transactions_order(
    const messages::TaggedBlock &tagged_block) const {
  const auto &block = tagged_block.block();
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
  messages::Assembly previous_assembly, previous_previous_assembly;
  const auto &block = tagged_block.block();

  if (!tagged_block.has_previous_assembly_id()) {
    LOG_INFO << "Failed check_block_author for block " << block.header().id()
             << " tagged block is missing field previous_assembly_id";
    return false;
  }
  if (!_ledger->get_assembly(tagged_block.previous_assembly_id(),
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

bool Consensus::verify_blocks() {
  std::lock_guard<std::recursive_mutex> lock(_verify_blocks_mutex);
  std::vector<messages::TaggedBlock> tagged_blocks;
  _ledger->get_unverified_blocks(&tagged_blocks);
  for (auto &tagged_block : tagged_blocks) {
    messages::TaggedBlock previous;
    if (!_ledger->get_block(tagged_block.block().header().previous_block_hash(),
                            &previous, false)) {
      // Something is wrong here
      throw std::runtime_error(
          "Could not find the previous block of an unverified block " +
          messages::to_json(tagged_block.block().header().id()));
    }
    assert(previous.branch() == messages::Branch::MAIN ||
           previous.branch() == messages::Branch::FORK);
    bool added_new_assembly = false;
    if (is_new_assembly(tagged_block, previous)) {
      const auto height =
          previous.block().header().height() / _config.blocks_per_assembly;
      _ledger->add_assembly(previous, height);
      added_new_assembly = true;
      tagged_block.mutable_previous_assembly_id()->CopyFrom(
          previous.block().header().id());

    } else if (!previous.has_previous_assembly_id()) {
      throw std::runtime_error(
          "A verified tagged_block is missing a previous_assembly_id " +
          messages::to_json(previous.block().header().id()));
    }
    auto assembly_id = added_new_assembly ? previous.block().header().id()
                                          : previous.previous_assembly_id();

    // Temporarily setup the previous_assembly_id so that check_block_author
    // can work
    tagged_block.mutable_previous_assembly_id()->CopyFrom(assembly_id);

    if (is_valid(tagged_block)) {
      _ledger->set_block_verified(tagged_block.block().header().id(),
                                  get_block_score(tagged_block), assembly_id);
    } else if (!_ledger->delete_block_and_children(
                   tagged_block.block().header().id())) {
      throw std::runtime_error("Failed to delete an invalid block");
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
  if (!previous.has_previous_assembly_id()) {
    throw std::runtime_error("Something is wrong here block " +
                             messages::to_json(previous.block().header().id()) +
                             " is missing the previous_assembly_id");
  }
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
  if (start_threads) {
    start_compute_pii_thread();
    start_update_heights_thread();
    start_miner_thread();
  }
}

Consensus::~Consensus() {
  _stop_compute_pii = true;
  _stop_update_heights = true;
  _stop_miner = true;
  if (_compute_pii_thread.joinable()) {
    _compute_pii_thread.join();
  }
  if (_update_heights_thread.joinable()) {
    _update_heights_thread.join();
  }
  if (_miner_thread.joinable()) {
    _miner_thread.join();
  }
}

bool Consensus::is_valid(
    const messages::TaggedTransaction &tagged_transaction) const {
  if (tagged_transaction.is_coinbase()) {
    return check_id(tagged_transaction) && check_coinbase(tagged_transaction);
  } else {
    return check_id(tagged_transaction) &&
           check_signatures(tagged_transaction) &&
           check_inputs(tagged_transaction) &&
           check_double_inputs(tagged_transaction) &&
           check_outputs(tagged_transaction);
  }
}

bool Consensus::is_valid(const messages::TaggedBlock &tagged_block) const {
  // This method should be only be called after the block has been inserted
  return check_block_id(tagged_block) &&
         check_block_transactions(tagged_block) &&
         check_block_size(tagged_block) &&
         check_transactions_order(tagged_block) &&
         check_block_height(tagged_block) &&
         check_block_timestamp(tagged_block) &&
         check_block_author(tagged_block) &&
         check_block_denunciations(tagged_block);
}

bool Consensus::add_transaction(const messages::Transaction &transaction) {
  messages::TaggedTransaction tagged_transaction;
  tagged_transaction.set_is_coinbase(false);
  tagged_transaction.mutable_transaction()->CopyFrom(transaction);
  return is_valid(tagged_transaction) &&
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

bool Consensus::add_block(const messages::Block &block) {
  return _ledger->insert_block(block) && verify_blocks() &&
         _ledger->update_main_branch() && add_double_mining(block);
}

void Consensus::start_compute_pii_thread() {
  _stop_compute_pii = false;
  if (!_compute_pii_thread.joinable()) {
    _compute_pii_thread = std::thread([this]() {
      while (!_stop_compute_pii) {
        std::vector<messages::Assembly> assemblies;
        _ledger->get_assemblies_to_compute(&assemblies);
        if (assemblies.size() > 0) {
          compute_assembly_pii(assemblies[0]);
          cleanup_expired_transactions();
        } else {
          std::this_thread::sleep_for(_config.compute_pii_sleep);
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
  };

  // Add the integrity before the pii because it is used in the pii computations
  for (const auto &[key_pub, score] : integrities.scores()) {
    _ledger->add_integrity(key_pub, assembly.id(), assembly.height(),
                           branch_path, score);
  }

  auto piis = pii.get_key_pubs_pii(assembly.height(), branch_path);
  if (piis.size() == 0) {
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
  };

  if (!_ledger->set_nb_key_pubs(assembly.id(), piis.size())) {
    LOG_WARNING
        << "During Pii computation failed to set nb_key_pubs for assembly "
        << assembly.id();
    return false;
  };
  if (!_ledger->set_finished_computation(assembly.id())) {
    LOG_WARNING << "During Pii computation failed to set "
                   "finished_computation for assembly "
                << assembly.id();
    return false;
  };
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
  const auto last_block_height = _ledger->height();
  messages::TaggedBlock last_block;
  _ledger->get_block(last_block_height, &last_block);
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
    }
    if (key_pubs_map.count(writer) > 0) {
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
      }
      if (key_pubs_map.count(writer) > 0) {
        heights->push_back({i, key_pubs_map[writer]});
      }
    }
  }

  return true;
}

bool Consensus::build_block(const crypto::Ecc &keys,
                            const messages::BlockHeight &height,
                            messages::Block *block) const {
  messages::TaggedBlock last_block;
  bool include_transactions = false;
  if (!_ledger->get_last_block(&last_block, include_transactions)) {
    return false;
  }
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

  _ledger->get_transaction_pool(block);
  if (!_ledger->cleanup_transactions(block)) {
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

  _ledger->add_denunciations(block, last_block.branch_path());

  messages::sort_transactions(block);
  messages::set_block_hash(block);
  crypto::sign(keys, block);

  return true;
}

void Consensus::start_update_heights_thread() {
  _stop_update_heights = false;
  if (!_update_heights_thread.joinable()) {
    _update_heights_thread = std::thread([this]() {
      while (!_stop_update_heights) {
        update_heights_to_write();
        std::this_thread::sleep_for(_config.update_heights_sleep);
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
  _stop_miner = false;
  if (!_miner_thread.joinable()) {
    _miner_thread = std::thread([this]() { mine_blocks(); });
  }
}

bool Consensus::mine_block(const messages::Block &block0) {
  std::lock_guard<std::mutex> lock(_heights_to_write_mutex);
  if (_heights_to_write.size() == 0) {
    return false;
  }
  const auto height = _heights_to_write[0].first;
  const auto key_pub_index = _heights_to_write[0].second;
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

  messages::Block new_block;
  if (!build_block(_keys[key_pub_index], height, &new_block)) {
    return false;
  }

  // Check that the newly created block isn't late
  if (new_block.header().timestamp().data() >= block_end) {
    return false;
  }

  _publish_block(new_block);
  add_block(new_block);
  LOG_INFO << "Mined block successfully with id " << new_block.header().id()
           << " with height " << new_block.header().height();
  return true;
}

void Consensus::mine_blocks() {
  messages::Block block0;
  if (!_ledger->get_block(0, &block0)) {
    throw std::runtime_error("Failed to get block0 in start_miner_thread");
  }
  while (!_stop_miner) {
    if (!mine_block(block0)) {
      std::this_thread::sleep_for(_config.miner_sleep);
    }
  }
}

void Consensus::cleanup_expired_transactions() {
  const auto &tip = _ledger->get_main_branch_tip();
  for (const auto &tagged_transaction : _ledger->get_transaction_pool()) {
    if (!is_unexpired(tagged_transaction.transaction(), tip.block())) {
      _ledger->delete_transaction(tagged_transaction.transaction().id());
    }
  }
}

}  // namespace consensus
}  // namespace neuro
