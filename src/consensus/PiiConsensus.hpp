#ifndef PiiConsensus_H
#define PiiConsensus_H

#include <cstdlib>
#include <memory>


#include "Pii.hpp"
#include "ledger/LedgerMongodb.hpp"
#include "Consensus.hpp"
#include "Forkmanager.hpp"


namespace neuro{
namespace consensus{

/**
*   \brief Consensus Calculate
*   for each block add , the consensus check if is valide by testit,
*
*/
class PiiConsensus : public Pii, public Consensus
{
    private:

        ledger::LedgerMongodb & _ledger;
        bool _valide_block;
        uint32_t _last_heigth_block;
        uint64_t _nonce_assembly = -1 ;

        Forkmanager _Forkmanager;

        void random_from_hashs()  ;
        uint32_t ramdon_at(int index , uint64_t nonce ) const;

    public:
        PiiConsensus(ledger::LedgerMongodb &ledger) : PiiConsensus(ledger, 2000) {}
        PiiConsensus(ledger::LedgerMongodb &ledger, uint32_t block_assembly);

        void add_block(const neuro::messages::Block &block);
        void add_blocks(const std::vector<neuro::messages::Block *> &blocks);
        std::string get_next_owner() const;

        bool check_owner(const neuro::messages::BlockHeader &blockheader) const;
};

}
}
#endif // PiiConsensus_H
