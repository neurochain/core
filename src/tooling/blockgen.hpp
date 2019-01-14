#include <boost/program_options.hpp>
#include <optional>
#include "crypto/Ecc.hpp"
#include "ledger/LedgerMongodb.hpp"
#include "messages/Message.hpp"

namespace po = boost::program_options;

namespace neuro {
namespace tooling {
namespace blockgen {

void coinbase(const crypto::EccPub &key_pub, const messages::NCCAmount &ncc,
              messages::Transaction *transaction,
              const std::string output_data = "");

messages::TaggedBlock gen_block0(const std::vector<crypto::Ecc> &keys,
                                 const messages::NCCAmount &ncc_block0);

void block0(uint32_t bots, const std::string &pathdir,
            const messages::NCCAmount &nccsdf, ledger::LedgerMongodb *ledger);

void testnet_blockg(uint32_t bots, const std::string &pathdir,
                    messages::NCCAmount &nccsdf);

bool blockgen_from_block(
    messages::Block *block, const messages::Block &last_block,
    const int32_t height, const uint64_t seed = 1,
    std::optional<neuro::messages::KeyPub> author = std::nullopt);

bool blockgen_from_last_db_block(
    messages::Block *block, std::shared_ptr<ledger::Ledger> ledger,
    const uint64_t seed, const int32_t new_height,
    std::optional<neuro::messages::KeyPub> author = std::nullopt,
    const int32_t last_height = 0);

void append_blocks(const int nb, std::shared_ptr<ledger::Ledger> ledger);

void append_fork_blocks(const int nb, std::shared_ptr<ledger::Ledger> ledger);

void create_empty_db(const messages::config::Database &config);

}  // namespace blockgen
}  // namespace tooling
}  // namespace neuro
