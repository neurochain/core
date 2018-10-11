#include "Pii.h"
namespace neuro {
namespace consensus {

Pii::Pii() {
  // ctor
  _Entropies.clear();
}

void Pii::addBlock(const PiiTransaction& p) {
  auto& pfrom = _Entropies[p.from];
  auto& pto = _Entropies[p.to];
  auto& tij1 = pfrom.entropie_Tij[p.to];
  auto& tij2 = pto.entropie_Tij[p.from];
  tij1.nbinput++;
  tij1.mtin += p.ncc * p.timencc * std::sqrt(pto.entropie);
  tij2.nboutput++;
  tij2.mtout += p.ncc * p.timencc * std::sqrt(pfrom.entropie);
  pfrom.sum_outputs++;
  pto.sum_inputs++;
}

void Pii::addBlocks(const std::vector<PiiTransaction>& thx) {
  // for( const piiThx &p: thx_)
  //    addBlock(p);
  for (const PiiTransaction& p : thx) {
    auto& pfrom = _Entropies[p.from];
    auto& pto = _Entropies[p.to];
    auto& tij1 = pfrom.entropie_Tij[p.to];
    auto& tij2 = pto.entropie_Tij[p.from];
    tij1.nbinput++;
    tij1.mtin += p.ncc * p.timencc * std::sqrt(pto.entropie);
    tij2.nboutput++;
    tij2.mtout += p.ncc * p.timencc * std::sqrt(pfrom.entropie);
    pfrom.sum_outputs++;
    pto.sum_inputs++;
  }
}

void Pii::calcul() {
  for (auto& p : _Entropies) {
    double epr1 = 0, epr2 = epr1;
    PiiCalculus& m = p.second;
    for (const auto& l : m.entropie_Tij) {
      //
      if (m.sum_outputs > 0 && l.second.mtin != 0 && l.second.nbinput != 0) {
        epr1 += std::log(l.second.mtin) * (l.second.nbinput / m.sum_outputs) *
                std::log2(l.second.nbinput / m.sum_outputs);
      }
      //
      if (m.sum_inputs > 0 && l.second.mtout != 0 && l.second.nboutput != 0) {
        epr2 += std::log(l.second.mtout) * (l.second.nboutput / m.sum_inputs) *
                std::log2(l.second.nboutput / m.sum_inputs);
      }
    }
    p.second.update(std::max(1.0, -0.5 * (epr1 + epr2)));
    //p.second.update(-0.5 * (epr1 + epr2));
  }

  // Order it
  std::vector<std::pair<std::string, PiiCalculus> > orderpii(_Entropies.begin(),
                                                             _Entropies.end());
  std::sort(orderpii.begin(), orderpii.end(), [](const auto& a, const auto& b) {
    return a.second.entropie > b.second.entropie;
  });

  int i = 0;
  for (auto& p : orderpii) {
    _owner_ordered.push_back(p.first);
    //std::cout << "p[" <<  i << "]=" << p.first << " = " << p.second.entropie << std::endl;
    i++;
    if (i > _assembly_owners) break;
  }
}

std::string Pii::operator()(uint32_t index) const {
  if (_owner_ordered.size() > index) return _owner_ordered[index];
  return std::string("");
}

void Pii::showresultat() {
  std::vector<std::pair<std::string, PiiCalculus> > orderpii(_Entropies.begin(),
                                                             _Entropies.end());
  std::sort(orderpii.begin(), orderpii.end(), [](const auto& a, const auto& b) {
    return a.second.entropie > b.second.entropie;
  });

  int i = 0;
  for (auto& p : orderpii) {
    std::cout << p.first << ":" << p.second.entropie;
    /*for(const auto&l : p.second.Erps)
        std::cout << std::get<0>(l) <<
       "["<<std::get<1>(l)<<":"<<std::get<2>(l)<<"] | ";*/
    std::cout << std::endl;
    i++;
    if (i > 57) break;
  }
}

}  // namespace consensus
}  // namespace neuro
