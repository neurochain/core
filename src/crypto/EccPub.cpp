#include "crypto/EccPub.hpp"
#include <iomanip>
#include <iostream>
#include "messages/Message.hpp"

namespace neuro {
namespace crypto {

EccPub::EccPub(const std::string &filepath) {
  if (!load(filepath)) {
    throw std::runtime_error("Failed to load public key from file.");
  }
}

EccPub::EccPub(const Buffer &pub_key) {
  if (!load(pub_key)) {
    throw std::runtime_error("Could not load key from buffer");
  }
}

EccPub::EccPub(const Key &key) : _key(key) {}

EccPub::Key *EccPub::key() { return &_key; }

bool EccPub::save(const std::string &filepath) const {
  CryptoPP::FileSink fs(filepath.c_str(), true);
  _key.Save(fs);

  return true;
}

bool EccPub::load(const std::string &filepath) {
  std::ifstream key_file(filepath, std::ifstream::binary);
  if (!key_file.is_open()) {
    return false;
  }
  std::vector<uint8_t> tmp(std::istreambuf_iterator<char>(key_file), {});
  Buffer buff(tmp.data(), tmp.size());
  return load(buff);
}

bool EccPub::load(const Buffer &buffer) {
  CryptoPP::StringSource array(reinterpret_cast<const byte *>(buffer.data()),
                               buffer.size(), true);
  _key.Load(array);
  return true;
}

bool EccPub::load(const uint8_t *data, const std::size_t size) {
  CryptoPP::StringSource array(reinterpret_cast<const byte *>(data), size,
                               true);
  _key.Load(array);
  return true;
}

bool EccPub::load(const messages::KeyPub &keypub) {
  if (keypub.has_raw_data()) {
    const auto &raw_data = keypub.raw_data();
    load(reinterpret_cast<const uint8_t *>(raw_data.data()), raw_data.size());
  } else if (keypub.has_hex_data()) {
    const auto &hex_data = keypub.hex_data();
    Buffer tmp(hex_data, Buffer::InputType::HEX);
    load(tmp);
  } else {
    // not possible since we have a oneof
    return false;
  }

  return true;
}

Buffer EccPub::save() const {
  Buffer tmp;
  std::string s;
  _key.Save(CryptoPP::StringSink(s).Ref());
  tmp.copy(s);
  return tmp;
}

bool EccPub::save(Buffer *buffer) const {
  std::string s;
  _key.Save(CryptoPP::StringSink(s).Ref());
  buffer->copy(s);
  return true;
}

bool EccPub::save(messages::KeyPub *key_pub) const {
  key_pub->set_type(messages::KeyType::ECP256K1);
  std::string s;
  _key.Save(CryptoPP::StringSink(s).Ref());
  key_pub->set_raw_data(s);
  return true;
}

bool EccPub::verify(const Buffer &data, const uint8_t *signature,
                    const std::size_t size) const {
  CryptoPP::ECDSA<CryptoPP::ECP, CryptoPP::SHA256>::Verifier verifier(_key);

  return verifier.VerifyMessage((const byte *)data.data(), data.size(),
                                (const byte *)signature, size);
}

bool EccPub::verify(const Buffer &data, const Buffer &signature) const {
  return verify(data, signature.data(), signature.size());
}

bool EccPub::operator==(const EccPub &key) const {
  CryptoPP::ByteQueue queue0;
  CryptoPP::ByteQueue queue1;
  _key.Save(queue0);
  key._key.Save(queue1);
  return (queue0 == queue1);
}

std::ostream &operator<<(std::ostream &os, const EccPub &pub) {
  messages::KeyPub k;
  pub.save(&k);
  os << k;
  return os;
}
}  // namespace crypto
}  // namespace neuro
