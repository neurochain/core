#ifndef NEURO_SRC_LEDGER_FILTER_HPP
#define NEURO_SRC_LEDGER_FILTER_HPP

#include "crypto/Ecc.hpp"
#include "messages/Message.hpp"

namespace neuro {
namespace ledger {

/**
 * \brief Criteria for Ledger::for_each function
 * Unset criterias are not applied
 */
class Filter {
 public:
  void lower_bound(const messages::BlockHeight);
  void upper_bound(const messages::BlockHeight);
  void input_key(const crypto::EccPub &key);
  void output_key_id(const messages::_KeyPub &key_pub);
};

}  // namespace ledger
}  // namespace neuro

#endif /* NEURO_SRC_LEDGER_FILTER_HPP */
