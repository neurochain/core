#ifndef FORKTREE_H
#define FORKTREE_H

#include <functional>
#include <memory>
#include <sstream>
#include <vector>
#include "messages.pb.h"

namespace neuro {
namespace consensus {

class ForkTree {
 public:
  enum BranchType { ForkBranch = 0, MainBranch };
  using trees = std::vector<std::shared_ptr<ForkTree>>;
  using Functor =
      std::function<void(const messages::Block, BranchType, BranchType)>;

 private:
  neuro::messages::Block _entry;
  BranchType _branch_origin, _branch_apply;

  uint64_t _weight;
  trees _tree;

  std::string printtree(const std::string &prefix, bool isLeft);
  inline bool equalme(const neuro::messages::Hash &block_id);
  uint64_t score();

 public:
  ForkTree() = default;

  ForkTree(neuro::messages::Block &block, uint64_t w = 0,
           BranchType branch = ForkBranch);

  void add(neuro::messages::Block &fork_block, BranchType branch);
  bool find_add(neuro::messages::Block &fork_block, BranchType branch);
  void set_branch(BranchType branch = ForkBranch) { _branch_apply = branch; }
  uint64_t update_branch();
  void for_each(Functor functor);

  friend std::ostream &operator<<(std::ostream &os, ForkTree &node_tree);
};

}  // namespace consensus
}  // namespace neuro
#endif  // FORKTREE_H
