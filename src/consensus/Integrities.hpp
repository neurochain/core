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

  void add_block(const messages::TaggedBlock &tagged_block);

  void add_integrity(const messages::Address &address,
                     const messages::IntegrityScore &score);
};

}  // namespace consensus
}  // namespace neuro

#endif
