#ifndef PIISUS_H
#define PIISUS_H

#include <cstdlib>
#include <memory>


#include "Pii.h"
#include "ledger/LedgerMongodb.hpp"
#include "Consensus.h"
#include "ForkSus.h"


namespace neuro{
namespace consensus{

/**
*   \brief Consensus Calculate
*   for each block add , the consensus check if is valide by testit,
*
*/
class PiiSus : public Pii, public Consensus
{
    private:
        ledger::LedgerMongodb & _ledger;
        void random_from_hashs()  ;
        uint32_t ramdon_at(int index , uint64_t nonce ) const;

        bool _valide_block;
        uint32_t _last_heigth_block;
        uint64_t _nonce_assembly = -1 ;

        ForkSus _forksus;

    public:
        PiiSus(ledger::LedgerMongodb &l) : PiiSus(l, 2000) {}
        PiiSus(ledger::LedgerMongodb &l, uint32_t block_assembly);

        void add_block(const neuro::messages::Block &b);
        void add_blocks(const std::vector<neuro::messages::Block *> &bs);
        std::string get_next_owner() const;

        bool check_owner(const neuro::messages::BlockHeader &bh) const;
};

}
}
#endif // PIISUS_H
