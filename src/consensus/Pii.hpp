#ifndef NEURO_SRC_CONCENSUS_PII_HPP
#define NEURO_SRC_CONCENSUS_PII_HPP

#include <algorithm>
#include <cmath>
#include <iostream>
#include <memory>
#include <tuple>
#include <unordered_map>
#include <vector>

#include "common/types.hpp"
#include "messages/Message.hpp"

namespace neuro {
namespace consensus {
///
/// \class pii class
///
class Pii {
 public:
  
  ///
  /// \struct piiThx "pii.h"
  /// \brief a simple struct of "transaction interaction" between 2 addr
  ///
  struct Transaction {
   public:
    std::string from; /*!< from the addr sender*/
    std::string to;   /*!< to the addr recever*/
    double ncc;       /*!< ncc the nombre of ncc*/
    uint64_t timencc; /*!< timencc time of ncc*/
  };

  /**
   *   \brief a help structure to store intermediate results
   */
  struct Calculus {
   public:
    struct PiiData {
      double nbinput, nboutput, mtin, mtout;
      PiiData() : nbinput(0), nboutput(0), mtin(0), mtout(0) {}
    };

    double entropie;  ///<! Ei (-1)
    // std::vector<std::tuple<double,double,double>> Erps;
    double sum_inputs, sum_outputs;

    std::unordered_map<Address, PiiData> entropie_Tij;

    Calculus() : entropie(1), sum_inputs(0), sum_outputs(0) {}
    Calculus(const Calculus &) = default;

    void update(const double new_entropie) {
      entropie = new_entropie;
      sum_inputs = sum_outputs = 0;
      entropie_Tij.clear();
    }

    friend bool operator>(const Calculus& l, const Calculus& r) {
      return l.entropie > r.entropie;
    }
  };

 protected:
  std::unordered_map<std::string, Calculus> _entropies;
  std::vector<std::string> _owner_ordered;
  int _assembly_owners = ASSEMBLY_MEMBERS_COUNT;
  uint32_t _assembly_blocks;

 public:
  /**
   *   \brief
   */
  Pii(){}

  /**
   * \brief add new piiThx
   * \param ptx the transaction interaction
   */
  void addBlock(const Transaction& piitransaction);

  /**
   *  \brief add more piiThx
   *  \param [in] vpt vector of piiTHx
   */
  void addBlocks(const std::vector<Transaction>& piitransactions);

  /**
   *   \brief Run the calcule of PII ( last step :) )
   */
  void calcul();

  /**
   *   \brief
   */
  std::string operator()(uint32_t index) const;

  void show_results();
};

}  // namespace consensus
}  // namespace neuro
#endif  // NEURO_SRC_CONCENSUS_PII_HPP
