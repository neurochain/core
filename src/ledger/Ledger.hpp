#ifndef NEURO_SRC_LEDGER_LEDGER_HPP
#define NEURO_SRC_LEDGER_LEDGER_HPP

#include <functional>
#include "ledger/Filter.hpp"
#include "messages.pb.h"

namespace neuro {
namespace ledger {

class Ledger {
 public:
  using Functor = std::function<bool(const messages::BlockHeader &,
                                     const messages::Transaction)>;

 private:
 public:
  virtual messages::BlockHeight height() const = 0;

  /**
   * \param [in] id id of the bloc
   * \param [out] block an allocated block structure
   * \return block state
   */
  virtual messages::Transaction::State get_block(const messages::BlockID &id,
						 messages::Block *block) = 0;

  /**
   * \param [in] height height of the bloc
   * \param [out] block an allocated block structure
   * \return block state
   */
  virtual messages::Transaction::State get_block(const messages::BlockHeight height,
                                           messages::Block *block) = 0;

  /**
   * \param [out] block an allocated block structure
   * \return block state
   */
  virtual messages::Transaction::State get_last_block(messages::Block *block) = 0;

  /**
   * \param [in] id id of the bloc
   * \param [out] header an allocated messages::BlockHeader structure
   * \return block state
   */
  virtual messages::BlockHeader::State get_block_header(
      const messages::BlockID &id, messages::BlockHeader *header) = 0;

  /**
   * \return false if could not push the block (exist || invalid || reject fork)
   */
  virtual bool push_block(const messages::Block &block) = 0;
  virtual bool delete_block(const messages::BlockID &id) = 0;

  /**
   * \brief call functor on every transaction that matches filter
   * \param [in] filter criterias that need to be match
   * \param [in,out] callback function
   * \return false if no match found
   */
  virtual bool for_each(Filter filter, Functor functor) = 0;

  /**
   * \param [in] id id of the transaction
   * \param [out] transaction allocated Transaction struture that will be filled
   * \return false if transaction not found
   */
  virtual messages::Transaction::State get_transaction(
      const messages::TransactionID &id,
      messages::Transaction *transaction) = 0;

  /**
   * \brief helper for Ledger::for_each function
   */
  virtual bool has_transaction(const messages::BlockID &block_id,
                               const messages::TransactionID &id) const = 0;

  /**
   * \brief helper for Ledger::for_each function
   */
  virtual bool has_transaction(
      const messages::TransactionID &transaction_id) const = 0;

  /**
   * \brief helper for Ledger::for_each function
   * \param transaction allocated Transaction struture that will be filled
   */
  virtual messages::Transaction::State transaction(
      const messages::BlockID &block_id,
      const messages::TransactionID &transaction_id,
      messages::Transaction *transaction) const = 0;

  virtual ~Ledger() {}
};

}  // namespace ledger
}  // namespace neuro

#endif /* NEURO_SRC_LEDGER_LEDGER_HPP */
