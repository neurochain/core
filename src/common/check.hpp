#ifndef NEURO_SRC_COMMON_CHECK_HPP
#define NEURO_SRC_COMMON_CHECK_HPP

#include <sstream>
#include <stdexcept>

#define CHECK(CONDITION, MSG) _CHECK(CONDITION, MSG, __FILE__, __LINE__)

#define _CHECK(CONDITION, MSG, FILE, LINE)                                 \
  if (!(CONDITION)) {                                                      \
    std::stringstream ss;                                                  \
    ss << "Error(" << FILE << ", " << LINE << "): " << (MSG) << std::endl; \
    throw std::runtime_error(ss.str());                                    \
  }

#endif