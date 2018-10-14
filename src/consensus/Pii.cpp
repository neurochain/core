#include "Pii.hpp"
namespace neuro {
namespace consensus {

void Pii::addBlock(const Transaction& piitransaction) {
  auto& pfrom = _entropies[piitransaction.from];
  auto& pto = _entropies[piitransaction.to];
  auto& tij1 = pfrom.entropie_Tij[piitransaction.to];
  auto& tij2 = pto.entropie_Tij[piitransaction.from];
  tij1.nbinput++;
  tij1.mtin +=
      piitransaction.ncc * piitransaction.timencc * std::sqrt(pto.entropie);
  tij2.nboutput++;
  tij2.mtout +=
      piitransaction.ncc * piitransaction.timencc * std::sqrt(pfrom.entropie);
  pfrom.sum_outputs++;
  pto.sum_inputs++;
}

void Pii::addBlocks(const std::vector<Transaction>& piitransactions) {
  // for( const piiThx &p: thx_)
  //    addBlock(p);
  for (const auto& piitransaction : piitransactions) {
    auto& pfrom = _entropies[piitransaction.from];
    auto& pto = _entropies[piitransaction.to];
    auto& tij1 = pfrom.entropie_Tij[piitransaction.to];
    auto& tij2 = pto.entropie_Tij[piitransaction.from];
    tij1.nbinput++;
    tij1.mtin +=
        piitransaction.ncc * piitransaction.timencc * std::sqrt(pto.entropie);
    tij2.nboutput++;
    tij2.mtout +=
        piitransaction.ncc * piitransaction.timencc * std::sqrt(pfrom.entropie);
    pfrom.sum_outputs++;
    pto.sum_inputs++;
  }
}

void Pii::calcul() {
  for (auto& p : _entropies) {
    double epr1 = 0;
    double epr2 = epr1;
    Calculus& m = p.second;
    for (const auto& l : m.entropie_Tij) {
      if (m.sum_outputs > 0 && l.second.mtin != 0 && l.second.nbinput != 0) {
        epr1 += std::log(l.second.mtin) * (l.second.nbinput / m.sum_outputs) *
                std::log2(l.second.nbinput / m.sum_outputs);
      }

      if (m.sum_inputs > 0 && l.second.mtout != 0 && l.second.nboutput != 0) {
        epr2 += std::log(l.second.mtout) * (l.second.nboutput / m.sum_inputs) *
                std::log2(l.second.nboutput / m.sum_inputs);
      }
    }
    // p.second.update(std::max(1.0, -0.5 * (epr1 + epr2)));
    p.second.update(-0.5 * (epr1 + epr2));
  }

  // Order it
  std::vector<std::pair<std::string, Calculus> > sorted_pii(_entropies.begin(),
                                                            _entropies.end());
  std::sort(sorted_pii.begin(), sorted_pii.end(),
            [](const auto& a, const auto& b) {
              return a.second.entropie > b.second.entropie;
            });

  int i = 0;
  for (auto& p : sorted_pii) {
    _owner_ordered.push_back(p.first);
    // std::cout << "p[" <<  i << "]=" << p.first << " = " << p.second.entropie
    // << std::endl;
    i++;
    if (i > _assembly_owners) break;
  }
}

std::string Pii::operator()(uint32_t index) const {
  if (_owner_ordered.size() > index) return _owner_ordered[index];
  return std::string("");
}

void Pii::show_results() {
  std::vector<std::pair<std::string, Calculus> > sorted_pii(_entropies.begin(),
                                                            _entropies.end());
  std::sort(sorted_pii.begin(), sorted_pii.end(),
            [](const auto& a, const auto& b) {
              return a.second.entropie > b.second.entropie;
            });

  int i = 0;
  for (auto& p : sorted_pii) {
    std::string t;
    messages::Hash id;
    id.ParseFromString(p.first);
    messages::to_json(id, &t);
    std::cout << t << ":" << p.second.entropie;
    std::cout << std::endl;
    i++;
    if (i > _assembly_owners) break;  // TODO remove magic number
  }
}

}  // namespace consensus
}  // namespace neuro
