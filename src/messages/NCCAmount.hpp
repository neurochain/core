#ifndef NEURO_SRC_MESSAGES_NCCAMOUNT_HPP
#define NEURO_SRC_MESSAGES_NCCAMOUNT_HPP

#include "common.pb.h"


namespace neuro {
namespace messages {

class NCCAmount : public _NCCAmount {
 public:
  NCCAmount() {}
  NCCAmount(const _NCCAmount &nccsdf) : _NCCAmount(nccsdf) {}

  NCCAmount(const uint64_t amount) { set_value(amount); }
};

}  // messages
}  // neuro

#endif /* NEURO_SRC_MESSAGES_NCCAMOUNT_HPP */