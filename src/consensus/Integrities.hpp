#ifndef NEURO_SRC_CONSENSUS_INTEGRITY_HPP
#define NEURO_SRC_CONSENSUS_INTEGRITY_HPP

#include "common.pb.h"
#include "consensus/Config.hpp"
#include "ledger/Ledger.hpp"
#include "messages.pb.h"
#include "messages/Message.hpp"

namespace neuro {
namespace consensus {

class Integrities {
 private:
  const Config _config;

  std::unordered_map<messages::_KeyPub, messages::IntegrityScore> _scores;

 public:
  explicit Integrities(const consensus::Config &config) : _config(config) {}

  Config config() const;

  std::unordered_map<messages::_KeyPub, messages::IntegrityScore> scores()
      const;

  void add_block(const messages::TaggedBlock &tagged_block);

  void add_integrity(const messages::_KeyPub &key_pub,
                     const messages::IntegrityScore &score);
};

}  // namespace consensus
}  // namespace neuro

#endif
