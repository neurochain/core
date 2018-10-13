#include "ForkTree.hpp"
namespace neuro {
namespace consensus {

ForkTree::ForkTree(neuro::messages::Block &block, uint64_t w,
                   BranchType branch) {
  _entry.CopyFrom(block);
  _weight = w + score();
  _branch_origin = branch;
  _tree.clear();
}

uint64_t ForkTree::score() {
  uint64_t s = 0;
  for (auto trx : _entry.transactions()) {
    for (auto output : trx.outputs()) {
      s +=
          output.value().value();  // std::atol(output.value().value().c_str());
    }
  }
  return s;
}

uint64_t ForkTree::add(neuro::messages::Block &fork_block, BranchType branch) {
  auto nfork = std::make_shared<ForkTree>(fork_block, _weight, branch);
  _tree.push_back(nfork);
  return nfork->_weight;
}

/*
void add(ForkTree &fork)
{
    _tree.push_back(fork);
}*/

bool ForkTree::find_add(neuro::messages::Block &fork_block, BranchType branch,
                        uint64_t &scoreofadd) {
  if (equalme(fork_block.header().previous_block_hash())) {
    scoreofadd = add(fork_block, branch);
    return true;
  }

  for (auto node : _tree) {
    if (node->find_add(fork_block, branch, scoreofadd)) {
      return true;
    }
  }
  return false;
}

void ForkTree::for_each(Functor functor) {
  functor(_entry, _branch_origin, _branch_apply);
  for (auto p : _tree) {
    p->for_each(functor);
  }
}

void ForkTree::set_branch(BranchType branch) {
  _branch_apply = branch;
  for (auto p : _tree) {
    p->set_branch(branch);
  }
}

uint64_t ForkTree::update_branch() {
  set_branch(MainBranch);
  if (_tree.size() == 0) {
    return _weight;
  }

  if (_tree.size() == 1) {
    return _tree[0]->update_branch();
  }

  uint64_t max_score = 0;
  int i = 0, max_i = -1;
  for (auto p : _tree) {
    auto res = p->update_branch();
    if (max_score <= res) {
      max_score = res;
      max_i = i;
    }
    p->set_branch();
    i++;
  }
  _tree[max_i]->set_branch(MainBranch);
  return max_score;
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
  if (true) {  //_tree.size() == 0) {
    ss << ":" << ((_branch_origin == MainBranch) ? "Main" : "Fork") << ":"
       << ((_branch_apply == MainBranch) ? "Main" : "Fork") << ":" << _weight;
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
