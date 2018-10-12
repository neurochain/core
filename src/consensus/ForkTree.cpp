#include "ForkTree.hpp"
namespace neuro {
namespace consensus {

ForkTree::ForkTree(neuro::messages::Block &block, uint64_t w,
                   BranchType branch) {
  _entry.CopyFrom(block);
  _weight = w + score();
  _branch = branch;
  _tree.clear();
}

uint64_t ForkTree::score() {
  uint64_t s = 0;
  for (auto trx : _entry.transactions()) {
    for (auto output : trx.outputs()) {
      s += output.value().value();  // std::atol(output.value().value().c_str());
    }
  }
  return s;
}

void ForkTree::add(neuro::messages::Block &fork_block, BranchType branch) {
  auto nfork = std::make_shared<ForkTree>(fork_block, _weight, branch);
  _tree.push_back(nfork);
}

/*
void add(ForkTree &fork)
{
    _tree.push_back(fork);
}*/

bool ForkTree::find_add(neuro::messages::Block &fork_block, BranchType branch) {
  if (equalme(fork_block.header().previous_block_hash())) {
    add(fork_block, branch);
    return true;
  }

  for (auto node : _tree) {
    if (node->find_add(fork_block, branch)) {
      return true;
    }
  }
  return false;
}

inline bool ForkTree::equalme(const neuro::messages::Hash &block_id) {
  return (_entry.header().id().data() == block_id.data());
}

std::ostream &operator<<(std::ostream &os, ForkTree &node_tree) {
  /*os << b._entry.header().height() << "->";
  for(auto k : b._tree)
      os << *k;*/
  os << node_tree.printtree("", false);
  return os;
}

std::string ForkTree::printtree(const std::string &prefix, bool isLeft) {
  if (_tree.size() == 1) {
    return _tree[0]->printtree(prefix, isLeft);
  }

  std::stringstream ss;
  ss << prefix;

  ss << (isLeft ? "├──" : "└──");

  // print the value of the node
  ss << _entry.header().height();
  if (_tree.size() == 0) {
    ss << ":" << ((_branch == MainBranch) ? "Main" : "Fork") << ":" << _weight;
  }
  ss << std::endl;

  // enter the next tree level - left and right branch
  int p = _tree.size();
  int i = 0;
  for (auto node : _tree) {
    if (i < (p - 1)) {
      ss << node->printtree(prefix + (isLeft ? "│   " : "    "), true);
    } else {
      ss << node->printtree(prefix + (isLeft ? "│   " : "    "), false);
    }
    i++;
  }

  return ss.str();
}

}  // namespace consensus
}  // namespace neuro
