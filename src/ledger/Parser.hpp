#ifndef NEURO_LEDGER_PARSER_HPP
#define NEURO_LEDGER_PARSER_HPP

namespace neuro {
namespace ledger {
/**
 *
 */
class Parser {
 public:
  virtual bool push_block(const Buffer &buffer, Ledger *ledger) = 0;
  virtual bool serialize(const Ledger &ledger, const BlockID id,
                         Buffer *buffer) = 0;
  virtual std::shared_ptr<Transaction> parse_transaction(
      const Buffer &buffer) = 0;
};

}  // namespace ledger
}  // namespace neuro

#endif /* NEURO_LEDGER_PARSER_HPP */
