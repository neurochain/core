#include "consensus/Integrities.hpp"

namespace neuro {
namespace consensus {

Config Integrities::config() const { return _config; }

std::unordered_map<messages::Address, messages::IntegrityScore>
Integrities::scores() const {
  return _scores;
};

void Integrities::add_block(const messages::TaggedBlock &tagged_block) {
  // Reward the block author
  const auto block_author_address =
      messages::Address(tagged_block.block().header().author().key_pub());
  add_integrity(block_author_address, _config.integrity_block_reward);

  // Denunciation
  for (const auto &denunciation : tagged_block.block().denunciations()) {
    const auto denunced_address =
        messages::Address(denunciation.block_author().key_pub());
    add_integrity(denunced_address, _config.integrity_double_mining);
    add_integrity(block_author_address, _config.integrity_denunciation_reward);
  }
}

void Integrities::add_integrity(const messages::Address &address,
                                const messages::IntegrityScore &score) {
  _scores.try_emplace(address, 0);
  _scores[address] += score;
}

}  // namespace consensus
}  // namespace neuro
