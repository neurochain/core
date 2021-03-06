#ifndef NEURO_SRC_TOOLING_SIMULATOR_HPP
#define NEURO_SRC_TOOLING_SIMULATOR_HPP

#include <cstdint>
#include <memory>

#include "consensus/Consensus.hpp"
#include "crypto/Ecc.hpp"
#include "ledger/LedgerMongodb.hpp"
#include "messages.pb.h"
#include "messages/NCCAmount.hpp"

namespace neuro {
namespace tooling {

class Simulator {
 private:
  messages::NCCAmount _ncc_block0;

 public:
  const float RATIO_TO_SEND = 0.1;
  std::vector<crypto::Ecc> keys;
  std::shared_ptr<neuro::ledger::LedgerMongodb> ledger;
  std::shared_ptr<neuro::consensus::Consensus> consensus;
  std::vector<messages::_KeyPub> key_pubs;
  std::unordered_map<messages::_KeyPub, uint32_t> key_pubs_indexes;

  Simulator(const std::string &db_url, const std::string &db_name,
            const int32_t nb_keys, const messages::NCCAmount &ncc_block0,
            const int32_t time_delta, const bool start_threads);

  static Simulator RealtimeSimulator(const std::string &db_url,
                                     const std::string &db_name,
                                     const int nb_keys,
                                     const messages::NCCAmount ncc_block0);

  static Simulator StaticSimulator(const std::string &db_url,
                                   const std::string &db_name,
                                   const int nb_keys,
                                   const messages::NCCAmount ncc_block0);

  messages::Block new_block(int nb_transactions,
                            const messages::TaggedBlock &last_block) const;
  messages::Block new_block(const messages::TaggedBlock &last_block) const;
  messages::Block new_block(int nb_transactions) const;
  messages::Block new_block() const;
  messages::Transaction random_transaction() const;
  void run(int nb_blocks, int transactions_per_block, bool compute_pii = true);
};

}  // namespace tooling
}  // namespace neuro

#endif
