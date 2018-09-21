#include "messages/Message.hpp"


namespace neuro {
namespace messages {

std::ostream & operator<< (std::ostream &os, const Packet &packet) {
  os << to_json(packet);
  return os;
}  

}  // messages
}  // neuro
