#ifndef NEURO_SRC_COMMON_BUFFER_HPP
#define NEURO_SRC_COMMON_BUFFER_HPP

#include <fstream>
#include <functional>
#include <iostream>
#include <string>
#include <string_view>
#include "common/types.hpp"

namespace neuro {

class Buffer : public std::vector<uint8_t> {
 public:
  enum class InputType { RAW, HEX /*, B64*/ };

 private:
  inline uint8_t char2uint(const char c) {
    if (c >= '0' && c <= '9') {
      return c - '0';
    }
    if (c >= 'A' && c <= 'F') {
      return c - 'A' + 10;
    }
    if (c >= 'a' && c <= 'f') {
      return c - 'a' + 10;
    }

    throw std::runtime_error("Bad input");
  }

  bool read_hex(const std::string &str);

 public:
  Buffer() = default;
  Buffer(const Buffer &) = default;
  Buffer(Buffer &&) = default;
  Buffer(size_type count, uint8_t v) : std::vector<uint8_t>(count, v) {}
  Buffer(const uint8_t *data, const std::size_t size) { copy(data, size); }
  Buffer(const char *data, const std::size_t size) {
    copy(reinterpret_cast<const uint8_t *>(data), size);
  }
  explicit Buffer(const std::initializer_list<uint8_t> init)
      : vector<uint8_t>(init) {}

  Buffer(const std::string &string,
         const InputType input_type = InputType::RAW);
  bool save(const std::string &filepath);
  void copy(const uint8_t *data, const std::size_t size);
  void copy(const std::string &string);
  void copy(const Buffer &buffer);
  void append(const Buffer &buffer);
  bool operator==(const Buffer &other);

  std::string str() const;
  std::string to_hex() const;
};

std::ostream &operator<<(std::ostream &os, const Buffer &buffer);

}  // namespace neuro

namespace std {
template <>
struct hash<neuro::Buffer> {
  typedef neuro::Buffer argument_type;
  typedef std::size_t result_type;
  result_type operator()(argument_type const &buffer) const noexcept {
    const auto string_view = std::string_view{
        reinterpret_cast<const char *>(buffer.data()), buffer.size()};
    const auto hash = std::hash<std::string_view>{}(string_view);
    return hash;
  }
};
}  // namespace std

#endif /* NEURO_SRC_COMMON_BUFFER_HPP */
