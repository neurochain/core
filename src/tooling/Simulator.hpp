#ifndef NEURO_SRC_TOOLING_SIMULATOR_HPP
#define NEURO_SRC_TOOLING_SIMULATOR_HPP

#include "consensus/Consensus.hpp"
#include "crypto/Ecc.hpp"
#include "ledger/LedgerMongodb.hpp"

namespace neuro {
namespace tooling {

class Simulator {
 private:
  messages::NCCAmount _ncc_block0;

 public:
  const float RATIO_TO_SEND = 0.5;
  std::vector<crypto::Ecc> keys;
  std::shared_ptr<neuro::ledger::LedgerMongodb> ledger;
  std::shared_ptr<neuro::consensus::Consensus> consensus;
  std::vector<messages::Address> addresses;
  std::unordered_map<messages::Address, uint32_t> addresses_indexes;

  Simulator(const std::string &db_url, const std::string &db_name,
            const int nb_keys, const messages::NCCAmount ncc_block0);

  messages::Block new_block(int nb_transactions,
                            const messages::TaggedBlock &last_block);
  messages::Transaction new_random_transaction();
  messages::Transaction send_ncc(const crypto::EccPriv &from_key_priv,
                                 const messages::Address &to_address,
                                 float ncc_ratio);
  messages::Transaction random_transaction();
  void run(int nb_blocks, int transactions_per_block);
};

}  // namespace tooling
}  // namespace neuro

#endif
