#include "consensus/Integrities.hpp"

namespace neuro {
namespace consensus {

Config Integrities::config() const { return _config; }

std::unordered_map<messages::_KeyPub, messages::IntegrityScore>
Integrities::scores() const {
  return _scores;
}

void Integrities::add_block(const messages::TaggedBlock &tagged_block) {
  // Reward the block author
  const auto block_author_key_pub =
      messages::_KeyPub(tagged_block.block().header().author().key_pub());
  add_integrity(block_author_key_pub, _config.integrity_block_reward);

  // Denunciation
  for (const auto &denunciation : tagged_block.block().denunciations()) {
    const auto denunced_key_pub =
        messages::_KeyPub(denunciation.block_author().key_pub());
    add_integrity(denunced_key_pub, _config.integrity_double_mining);
    add_integrity(block_author_key_pub, _config.integrity_denunciation_reward);
  }
}

void Integrities::add_integrity(const messages::_KeyPub &key_pub,
                                const messages::IntegrityScore &score) {
  _scores.try_emplace(key_pub, 0);
  _scores[key_pub] += score;
}

}  // namespace consensus
}  // namespace neuro
