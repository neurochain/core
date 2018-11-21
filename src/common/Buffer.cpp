#include <iomanip>

#include <iostream>
#include <iterator>
#include "common/Buffer.hpp"

namespace neuro {

Buffer::Buffer(const std::string &string, const InputType input_type) {
  switch (input_type) {
    case InputType::HEX:
      if (!read_hex(string)) {
        throw std::runtime_error("Bad input size");
      }
      break;
    case InputType::RAW:
      copy(string);
      break;
    default:
      throw std::runtime_error("Unknown type: inconsistent implementation.");
  }
}

void Buffer::save(const std::string &filepath) {
  std::ofstream of(filepath, std::ios::binary);
  of.write(reinterpret_cast<const char *>(data()), size());
}

void Buffer::copy(const uint8_t *data, const std::size_t size) {
  resize(size);
  std::copy(data, data + size, begin());
}

void Buffer::copy(const std::string &string) {
  resize(string.size());
  std::copy(string.begin(), string.end(), begin());
}

void Buffer::copy(const Buffer &buffer) {
  resize(buffer.size());
  std::copy(buffer.begin(), buffer.end(), begin());
}

bool Buffer::operator==(const Buffer &other) {
  if (other.size() != this->size()) {
    return false;
  }
  size_t inner_size = this->size();
  for (size_t i = 0; i < inner_size; i += 1) {
    if (*(this->data() + i) != other[i]) {
      return false;
    }
  }

  return true;
}

std::string Buffer::str() const {
  std::stringstream ss;
  ss << this;
  return ss.str();
}

std::ostream &operator<<(std::ostream &os, const Buffer &buffer) {
  std::ios_base::fmtflags f(os.flags());
  os << std::setfill('0') << std::setw(2) << std::hex;
  std::copy(buffer.cbegin(), buffer.cend(), std::ostream_iterator<int>(os, ""));
  os.flags(f);

  return os;
}

bool Buffer::read_hex(const std::string &str) {
  if ((str.size() % 2) == 1) {
    return false;
  }
  clear();
  reserve(str.size() / 2);
  auto it = str.begin(), end = str.end();
  while (it < end) {
    push_back((char2uint(*it) << 4) + char2uint(*(it + 1)));
    it += 2;
  }
  return true;
}

}  // namespace neuro
