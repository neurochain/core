#ifndef FORKSUS_H
#define FORKSUS_H

#include "messages.pb.h"
#include "ledger/LedgerMongodb.hpp"


namespace neuro {
namespace consensus {

///
/// H1 : PII ✓
/// H2 : BLocks in Ledger ✓
/// H3 : Cancel BLock if prev in other block
///     ==> we have 8 block , 9 ==> 7 , Cancel 9
///
class ForkSus {
  public:
    enum ForkStatus {
        Non_Fork = 0, ///!< life is good
        Dual_Block, ///!< the same owenr do 2 version of block :)
        VS_Block,  ///!< 1 of owner is fake by PII cal : the H1 is the caluclet of Pii is just
        Fork_Time,
        Olb_Block,
        Timeout_Block,
        Separate_Block,
    };


  protected:
  private:

    class Forktree {
      public:
        using trees = std::vector<std::shared_ptr<Forktree>>;
      private:
        neuro::messages::Block _entry;
        trees _tree;

      public:
        Forktree() = default;

        Forktree(neuro::messages::Block &block) {
            _entry.CopyFrom(block);
            _tree.clear();
        }

        void add(neuro::messages::Block &fork) {
            auto nfork = std::make_shared<Forktree>(fork);
            _tree.push_back(nfork);
        }

        /*
        void add(Forktree &fork)
        {
            _tree.push_back(fork);
        }*/

        bool find_add(neuro::messages::Block &fork) {

            if ( equalme(fork.header().previous_block_hash()) ) {
                add(fork);
                return true;
            }

            for(auto f : _tree) {
                if ( f->find_add(fork) ) {
                    return true;
                }
            }
            return false;
        }

        inline bool equalme(const neuro::messages::Hash &b) {
            return (_entry.header().id().data() == b.data());
        }

        friend std::ostream &operator<<(std::ostream &os, Forktree &b) {
            /*os << b._entry.header().height() << "->";
            for(auto k : b._tree)
                os << *k;*/
            os << b.printBT("",false);
            return os;
        }


        std::string printBT(const std::string& prefix, bool isLeft) {

            if ( _tree.size() == 1 ) {
                return _tree[0]->printBT( prefix, isLeft);
            }

            std::stringstream ss;
            ss << prefix;

            ss << (isLeft ? "├──" : "└──" );

            // print the value of the node
            ss << _entry.header().height() << std::endl;

            // enter the next tree level - left and right branch
            int p = _tree.size();
            int i = 0;
            for( auto k : _tree ) {
                if ( i < (p - 1)) {
                    ss << k->printBT( prefix + (isLeft ? "│   " : "    "), true);
                } else {
                    ss << k->printBT( prefix + (isLeft ? "│   " : "    "), false);
                }
                i++;
            }

            return ss.str();
        }
    };

  public:
    ForkSus();
    virtual ~ForkSus();

    ForkStatus fork_status(const neuro::messages::BlockHeader &bh,
                           const neuro::messages::BlockHeader &prev_bh,
                           const neuro::messages::BlockHeader &last_bh);

    void fork_results( neuro::ledger::LedgerMongodb &ledger);
};

} // Consensus
} // Neuro

#endif // FORKSUS_H
