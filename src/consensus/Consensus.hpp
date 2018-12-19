#ifndef NEURO_SRC_CONSENSUS_CONSENSUS_HPP
#define NEURO_SRC_CONSENSUS_CONSENSUS_HPP

namespace neuro {
namespace consensus {

class Consensus {
 private:
    std::shared_ptr<ledger::Ledger> _ledger;
    std::shared_ptr<networking::Networking> _networking; // Do we really need the networking???

 public:
  Consensus(std::shared_ptr<ledger::Ledger> ledger,
	    std::shared_ptr<networking::Networking> networking) {}

  void build_block(){}

  bool is_valid(const messages::Transaction &transaction, const messages::TaggedBlock &tip) {
      // Check that all inputs are unspent
      for (const auto &input: inputs) {
          const auto transaction = _ledger->get_trasnaction(input.id());
          if (!_ledger->is_unspent_output(input.id(), input.output_id(), tip) {
            return false;
          }
      }

      // Check that the output + fee = input
      // Check the signatures
      // Check the id
  }

  /**
   * \brief Add transaction to transaction pool
   * \param [in] transaction
   */
  bool add_transaction(const messages::Transaction &transaction){ return false; }

  /**
   * \brief Add block and compute Pii
   * \param [in] block block to add
   */
  bool add_block(const messages::Block &block){ return false; }

  // /**
  //  * \brief Get the next addr to build block
  //  * \return Address of next block builder
  //  */
  // messages::Address get_next_owner() const{}

  // /**
  //  * \brief Check if the owner of the block is the right one
  //  * \param [in] block_header the block header
  //  * \return false if the owner is not the right one
  //  */
  // bool check_owner(const messages::BlockHeader &block_header) const{}

  void add_wallet_key_pair(const std::shared_ptr<crypto::Ecc> wallet){}
};
  

}  // consensus  
}  // neuro

#endif /* NEURO_SRC_CONSENSUS_CONSENSUS_HPP */
