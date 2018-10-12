#ifndef FORKTREE_H
#define FORKTREE_H

#include "messages.pb.h"
#include <sstream>
#include <vector>
#include <memory>

namespace neuro {
namespace consensus {

class Forktree {
  public:
    using trees = std::vector<std::shared_ptr<Forktree>>;

    enum BranchType {
        ForkBranch = 0,
        MainBranch
    };

  private:

    neuro::messages::Block _entry;
    BranchType _branch;
    uint64_t _weight;
    trees _tree;

  public:

    Forktree() = default;

    Forktree(neuro::messages::Block &block, uint64_t w = 0, BranchType branch = ForkBranch);

    uint64_t score();

    void add(neuro::messages::Block &fork_block, BranchType branch);

    bool find_add(neuro::messages::Block &fork_block, BranchType branch);

    inline bool equalme(const neuro::messages::Hash &block_id);

    friend std::ostream &operator<<(std::ostream &os, Forktree &node_tree);

    std::string printtree(const std::string& prefix, bool isLeft);
};


} // Consensus
} // Neuro
#endif // FORKTREE_H
