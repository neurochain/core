#ifndef NEURO_SRC_MESSAGES_HEADERPATTERN_HPP
#define NEURO_SRC_MESSAGES_HEADERPATTERN_HPP

namespace neuro {
namespace networking {
namespace tcp {

struct __attribute__((__packed__)) HeaderPattern {
  uint32_t size;
  uint8_t signature[64];
};

}  // namespace tcp
}  // namespace networking
}  // namespace neuro

#endif /* NEURO_SRC_MESSAGES_HEADERPATTERN_HPP */
