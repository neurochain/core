#ifndef NEURO_SRC_CONSENSUS_INTEGRITY_HPP
#define NEURO_SRC_CONSENSUS_INTEGRITY_HPP

#include "consensus/Config.hpp"
#include "ledger/Ledger.hpp"

namespace neuro {
namespace consensus {

class Integrities {
 public:
  const Config config;
  Integrities(const consensus::Config &config) : config(config) {}

  std::unordered_map<messages::Address, messages::IntegrityScore> scores;

  void add_block(const messages::TaggedBlock &tagged_block) {
    // Reward the block author
    auto block_author_address =
        messages::Address(tagged_block.block().header().author().key_pub());
    add_integrity(block_author_address, config.integrity_block_reward);

    // Denunciation
    for (const auto denunciation : tagged_block.block().denunciations()) {
      auto denunced_address =
          messages::Address(denunciation.block_author().key_pub());
      add_integrity(denunced_address, config.integrity_double_mining);
      add_integrity(block_author_address, config.integrity_denunciation_reward);
    }
  }

  void add_integrity(const messages::Address &address,
                     const messages::IntegrityScore &score) {
    scores.try_emplace(address, 0);
    scores[address] += score;
  }
};

}  // namespace consensus
}  // namespace neuro

#endif
