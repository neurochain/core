#include "messages/Address.hpp"
#include <cryptopp/config.h>
#include <sstream>
#include "crypto/Hash.hpp"

namespace neuro {
namespace messages {

const int BASE = 58;
const std::string ALPHABET[BASE] = {
    "1", "2", "3", "4", "5", "6", "7", "8", "9", "A", "B", "C", "D", "E", "F",
    "G", "H", "J", "K", "L", "M", "N", "P", "Q", "R", "S", "T", "U", "V", "W",
    "X", "Y", "Z", "a", "b", "c", "d", "e", "f", "g", "h", "i", "j", "k", "m",
    "n", "o", "p", "q", "r", "s", "t", "u", "v", "w", "x", "y", "z"};

const std::unordered_map<char, int> REVERSE_ALPHABET = {
    {'1', 0},  {'2', 1},  {'3', 2},  {'4', 3},  {'5', 4},  {'6', 5},
    {'7', 6},  {'8', 7},  {'9', 8},  {'A', 9},  {'B', 10}, {'C', 11},
    {'D', 12}, {'E', 13}, {'F', 14}, {'G', 15}, {'H', 16}, {'J', 17},
    {'K', 18}, {'L', 19}, {'M', 20}, {'N', 21}, {'P', 22}, {'Q', 23},
    {'R', 24}, {'S', 25}, {'T', 26}, {'U', 27}, {'V', 28}, {'W', 29},
    {'X', 30}, {'Y', 31}, {'Z', 32}, {'a', 33}, {'b', 34}, {'c', 35},
    {'d', 36}, {'e', 37}, {'f', 38}, {'g', 39}, {'h', 40}, {'i', 41},
    {'j', 42}, {'k', 43}, {'m', 44}, {'n', 45}, {'o', 46}, {'p', 47},
    {'q', 48}, {'r', 49}, {'s', 50}, {'t', 51}, {'u', 52}, {'v', 53},
    {'w', 54}, {'x', 55}, {'y', 56}, {'z', 57}};

std::string encode_base58(const CryptoPP::Integer &num,
                          const std::string &version) {
  std::stringstream encoded_stream;
  CryptoPP::Integer div, mod, remainder(num);
  do {
    div = remainder / BASE;
    mod = (remainder - (BASE * div));
    encoded_stream << ALPHABET[mod.ConvertToLong()];
    remainder = div;
  } while (remainder > 0);
  std::string encoded = encoded_stream.str();
  std::reverse(encoded.begin(), encoded.end());
  return version + encoded;
}

std::string encode_base58(const Buffer &buffer, const std::string &version) {
  CryptoPP::Integer num;
  num.Decode(buffer.data(), buffer.size());
  return encode_base58(num, version);
}

std::string encode_base58(const std::string &data, const std::string &version) {
  CryptoPP::Integer num;
  num.Decode(reinterpret_cast<const CryptoPP::byte *>(data.data()),
             data.size());
  return encode_base58(num, version);
}

CryptoPP::Integer decode_base58(const std::string &encoded_number) {
  CryptoPP::Integer num(CryptoPP::Integer::Zero());
  CryptoPP::Integer radix(CryptoPP::Integer::One());

  for (int i = encoded_number.size() - 1; i >= 0; --i) {
    const auto character = REVERSE_ALPHABET.at(encoded_number[i]) * radix;
    num += character;
    radix *= BASE;
  }

  return num;
}

void Address::init(const messages::_KeyPub &key_pub) {
  auto address = encode_base58(crypto::hash_sha3_256(key_pub), "N");
  address.resize(_hash_size);
  auto checksum = encode_base58(crypto::hash_sha3_256(address));
  checksum.resize(_checksum_size);
  address += checksum;
  set_data(address);
}

Address::Address() : _Address() {}
Address::Address(const messages::_KeyPub &key_pub) { init(key_pub); }

Address::Address(const crypto::KeyPub &ecc_pub) {
  messages::_KeyPub key_pub;
  ecc_pub.save(&key_pub);
  init(key_pub);
}

Address::Address(const messages::_Address &address) : _Address(address) {}
Address::Address(const std::string &str) { set_data(str); }

Address Address::random() {
  crypto::Ecc ecc;
  return Address(ecc.key_pub());
}

bool Address::verify() const {
  if (!has_data()) {
    return false;
  }

  if (data().size() < (_hash_size + _checksum_size)) {
    return false;
  }

  const auto address(data().substr(0, _hash_size));
  const auto checksum_from_key(data().substr(_hash_size, _checksum_size));
  auto checksum_computed = encode_base58(crypto::hash_sha3_256(address));
  checksum_computed.resize(_checksum_size);

  if (checksum_computed != checksum_from_key) {
    LOG_INFO << "Key checksum error";
    return false;
  }

  return true;
}

Address::operator bool() const { return verify(); }

}  // namespace messages
}  // namespace neuro
