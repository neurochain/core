#ifndef NEURO_SRC_MESSAGES_PEER_HPP
#define NEURO_SRC_MESSAGES_PEER_HPP

#include <mutex>
#include "common.pb.h"

namespace neuro {
namespace messages {

class Peer: public _Peer {
 private:
  mutable std::mutex _mutex;
  
 public:

  void set_status(::neuro::messages::Peer_Status value) {
    std::lock_guard<std::mutex> lock_queue(_mutex);
    _Peer::set_status(value);
  }

};
  

}  // messages
}  // neuro

#endif /* NEURO_SRC_MESSAGES_PEER_HPP */
