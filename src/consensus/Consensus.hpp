#ifndef NEURO_SRC_CONSENSUS_CONSENSUS_HPP
#define NEURO_SRC_CONSENSUS_CONSENSUS_HPP

#include <unordered_set>
#include "consensus/Integrities.hpp"
#include "consensus/Pii.hpp"
#include "crypto/Sign.hpp"
#include "ledger/Ledger.hpp"
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

using PublishBlock = std::function<void(const messages::Block &block)>;
using KeyPubIndex = uint32_t;

class Consensus {
 private:
  const Config _config;
  std::shared_ptr<ledger::Ledger> _ledger;
  const std::vector<crypto::Ecc> &_keys;
  std::vector<messages::_KeyPub> _key_pubs;
  const PublishBlock _publish_block;
  std::atomic<bool> _stop_compute_pii;
  std::thread _compute_pii_thread;
  std::vector<std::pair<messages::BlockHeight, KeyPubIndex>> _heights_to_write;
  std::mutex _heights_to_write_mutex;
  std::recursive_mutex _verify_blocks_mutex;
  std::optional<messages::AssemblyID> _previous_assembly_id;
  std::optional<messages::AssemblyHeight> _current_assembly_height;
  std::atomic<bool> _stop_update_heights;
  std::thread _update_heights_thread;
  std::atomic<bool> _stop_miner;
  std::thread _miner_thread;

  bool check_outputs(
      const messages::TaggedTransaction tagged_transaction) const;

  bool check_signatures(
      const messages::TaggedTransaction &tagged_transaction) const;

  bool check_id(const messages::TaggedTransaction &tagged_transaction) const;

  bool check_double_inputs(
      const messages::TaggedTransaction &tagged_transaction) const;

  bool check_coinbase(
      const messages::TaggedTransaction &tagged_transaction) const;

  messages::NCCAmount block_reward(const messages::BlockHeight height,
                                   const messages::NCCValue fees) const;

  bool check_block_id(const messages::TaggedBlock &tagged_block) const;

  bool is_unexpired(const messages::Transaction &transaction,
                    const messages::Block &block) const;

  bool is_block_transaction_valid(
      const messages::TaggedTransaction &tagged_transaction,
      const messages::Block &block) const;

  bool check_block_transactions(
      const messages::TaggedBlock &tagged_block) const;

  bool check_block_size(const messages::TaggedBlock &tagged_block) const;

  bool check_transactions_order(
      const messages::TaggedBlock &tagged_block) const;

  bool check_block_height(const messages::TaggedBlock &tagged_block) const;

  bool check_block_timestamp(const messages::TaggedBlock &tagged_block) const;

  bool check_block_author(const messages::TaggedBlock &tagged_block) const;

  bool check_block_denunciations(
      const messages::TaggedBlock &tagged_block) const;

  bool check_balances(const messages::TaggedBlock &tagged_block) const;

  messages::BlockScore get_block_score(
      const messages::TaggedBlock &tagged_block) const;

  bool verify_blocks();

  bool is_new_assembly(const messages::TaggedBlock &tagged_block,
                       const messages::TaggedBlock &previous) const;

  bool mine_block(const messages::Block &block0);

 public:
  Consensus(std::shared_ptr<ledger::Ledger> ledger,
            const std::vector<crypto::Ecc> &keys,
            const PublishBlock &publish_block, bool start_threads = true);

  Consensus(std::shared_ptr<ledger::Ledger> ledger,
            const std::vector<crypto::Ecc> &keys,
            const std::optional<Config> &config,
            const PublishBlock &publish_block, bool start_threads = true);

  Consensus(std::shared_ptr<ledger::Ledger> ledger,
            const std::vector<crypto::Ecc> &keys, const Config &config,
            const PublishBlock &publish_block, bool start_threads = true);

  Config config() const;

  void init(bool start_threads);

  ~Consensus();

  bool is_valid(const messages::TaggedTransaction &tagged_transaction) const;

  bool is_valid(const messages::TaggedBlock &tagged_block) const;

  /**
   * \brief Add transaction to transaction pool
   * \param [in] transaction
   */
  bool add_transaction(const messages::Transaction &transaction);

  bool add_double_mining(const messages::Block &block);

  /**
   * \brief Verify that a block is valid and insert it
   * \param [in] block block to add
   */
  bool add_block(const messages::Block &block);

  /**
   * \brief Check if there is any assembly to compute and if so starts the
   * computation in another thread
   */
  void start_compute_pii_thread();

  bool compute_assembly_pii(const messages::Assembly &assembly);

  bool get_block_writer(const messages::Assembly &assembly,
                        const messages::BlockHeight &height,
                        messages::_KeyPub *key_pub) const;

  messages::BlockHeight get_current_height() const;
  messages::AssemblyHeight get_current_assembly_height() const;

  bool get_heights_to_write(
      const std::vector<messages::_KeyPub> &key_pubs,
      std::vector<std::pair<messages::BlockHeight, KeyPubIndex>> *heights)
      const;

  bool cleanup_transactions(messages::Block *block) const;

  bool build_block(const crypto::Ecc &keys, const messages::BlockHeight &height,
                   messages::Block *block) const;

  void start_update_heights_thread();

  void update_heights_to_write();

  void start_miner_thread();

  void mine_blocks();

  void cleanup_expired_transactions();

  friend class neuro::consensus::tests::Consensus;
  friend class neuro::consensus::tests::RealtimeConsensus;
  friend class neuro::tooling::tests::RealtimeSimulator;
};

}  // namespace consensus
}  // namespace neuro

#endif /* NEURO_SRC_CONSENSUS_CONSENSUS_HPP */
