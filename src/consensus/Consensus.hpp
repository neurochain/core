#ifndef CONSENSUS_H
#define CONSENSUS_H

#include <string>
#include <map>

#include "messages.pb.h"

namespace neuro{
namespace consensus{

class Consensus
{
    public:
        /**
        * \brief Add block for PII calcul
        * \param [in] block block to add
        */
        virtual void add_block(const neuro::messages::Block &block) = 0;

        /**
        * \brief Add blocks for PII calcul
        * \param [in] bs array of block
        */
        virtual void add_blocks(const std::vector<neuro::messages::Block *> &blocks) = 0;

        /**
        * \brief Get the next addr to build block
        * \return Address of next block builder
        */
        virtual std::string get_next_owner() const = 0;

        /**
        * \brief Check if the owner of the block is the right one
        * \param [in] bh the block header
        * \return false if the owner is not the right one
        */
        virtual bool check_owner(const neuro::messages::BlockHeader &blockheader) const = 0;

};

} // consensus
} // neuro
#endif // CONSENSUS_H
