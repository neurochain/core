#ifndef NEURO_SRC_CONSENSUS_INTEGRITY_HPP
#define NEURO_SRC_CONSENSUS_INTEGRITY_HPP

#include "consensus/Config.hpp"
#include "ledger/Ledger.hpp"
#include "messages/Message.hpp"

namespace neuro {
namespace consensus {

class Integrities {
 private:
  const Config _config;

  std::unordered_map<messages::Address, messages::IntegrityScore> _scores;

 public:
  Integrities(const consensus::Config &config) : _config(config) {}

  Config config() const;

  std::unordered_map<messages::Address, messages::IntegrityScore> scores()
      const;

  void add_block(const messages::TaggedBlock &tagged_block);

  void add_integrity(const messages::Address &address,
                     const messages::IntegrityScore &score);
};

}  // namespace consensus
}  // namespace neuro

#endif
