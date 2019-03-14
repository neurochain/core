#include <cryptopp/hex.h>
#include <messages/Address.hpp>
#include <sstream>

namespace neuro {
namespace messages {

const int BASE = 58;
const std::string ALPHABET[BASE] = {
    "1", "2", "3", "4", "5", "6", "7", "8", "9", "A", "B", "C", "D", "E", "F",
    "G", "H", "J", "K", "L", "M", "N", "P", "Q", "R", "S", "T", "U", "V", "W",
    "X", "Y", "Z", "a", "b", "c", "d", "e", "f", "g", "h", "i", "j", "k", "m",
    "n", "o", "p", "q", "r", "s", "t", "u", "v", "w", "x", "y", "z"};

std::string encode_base58(const CryptoPP::Integer &num,
                          const std::string &version) {
  std::stringstream encoded_stream;
  CryptoPP::Integer div, mod, remainder(num);
  while (remainder > 0) {
    div = remainder / BASE;
    mod = (remainder - (BASE * div));
    encoded_stream << ALPHABET[mod.ConvertToLong()];
    remainder = div;
  }
  std::string encoded = encoded_stream.str();
  std::reverse(encoded.begin(), encoded.end());
  return version + encoded;
}

std::string encode_base58(const Buffer &buffer, const std::string &version) {
  CryptoPP::Integer num;
  num.Decode(buffer.data(), buffer.size());
  return encode_base58(num, version);
}

void Address::init(const messages::KeyPub &key_pub) {
  auto key = crypto::hash_sha3_256(key_pub);
  auto checksum = crypto::hash_sha3_256(key);
  checksum.resize(_checksum_size);
  key.append(checksum);
  set_data(encode_base58(key, "N").substr(0, _address_size));
}

Address::Address(const messages::KeyPub &key_pub) { init(key_pub); }

Address::Address(const crypto::EccPub &ecc_pub) {
  messages::KeyPub key_pub;
  ecc_pub.save(&key_pub);
  init(key_pub);
}

Address::Address(const messages::_Address &address) : _Address(address) {}

Address::Address() : _Address() {}

Address Address::random() {
  crypto::Ecc ecc;
  return Address(ecc.public_key());
}

bool Address::verify() const {
  if (!has_data()) {
    return false;
  }

  if (data().size() != (_hash_size + _checksum_size)) {
    return false;
  }

  const Buffer key(data().substr(0, _hash_size));
  const Buffer checksum_from_key(data().substr(_hash_size, _checksum_size));
  auto checksum_computed = crypto::hash_sha3_256(key);

  if (checksum_computed != checksum_from_key) {
    LOG_INFO << "Key checksum error";
    return false;
  }

  return true;
}

Address::operator bool() const { return verify(); }

}  // namespace messages
}  // namespace neuro
