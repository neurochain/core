#include "common/types.hpp"

namespace neuro {

std::time_t _time = 0;
bool _fake_time = false;

std::time_t time() {
  if (!_fake_time) {
    return std::time(nullptr);
  } else {
    return _time;
  }
}
std::time_t time(const std::time_t increment) {
  _time += increment;
  return _time;
}

}  // namespace neuro
