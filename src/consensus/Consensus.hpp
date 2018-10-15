#ifndef CONSENSUS_H
#define CONSENSUS_H

#include <vector>

#include "messages/Message.hpp"

namespace neuro {
namespace consensus {

class Consensus {
 public:
  virtual void build_block() = 0;
  /**
   * \brief Add transaction in pool transactions
   * \param [in] transaction
   */
  virtual void add_transaction(const messages::Transaction &transaction) = 0;
  /**
   * \brief Add block for PII calcul
   * \param [in] block block to add
   */
  virtual void add_block(const messages::Block &block) = 0;

  /**
   * \brief Add blocks for PII calcul
   * \param [in] bs array of block
   */
  virtual void add_blocks(const std::vector<messages::Block *> &blocks) = 0;

  /**
   * \brief Get the next addr to build block
   * \return Address of next block builder
   */
  virtual Address get_next_owner() const = 0;

  /**
   * \brief Check if the owner of the block is the right one
   * \param [in] bh the block header
   * \return false if the owner is not the right one
   */
  virtual bool check_owner(const messages::BlockHeader &block_header) const = 0;
};

}  // namespace consensus
}  // namespace neuro
#endif  // CONSENSUS_H
