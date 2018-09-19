#include "crypto/EccPub.hpp"
#include <iomanip>
#include <iostream>

namespace neuro {
namespace crypto {

EccPub::EccPub(const std::string &filepath) {
  load(filepath);
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
  CryptoPP::FileSource fs(filepath.c_str(), true);
  _key.Load(fs);

  return true;
}

bool EccPub::load(const Buffer &buffer) {
  CryptoPP::StringSource array(reinterpret_cast<const byte *>(buffer.data()),
                               buffer.size(), true);
  _key.Load(array);
  return true;
}

bool EccPub::save(Buffer *buffer) const {
  std::string s;
  _key.Save(CryptoPP::StringSink(s).Ref());
  buffer->copy(s);
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

std::ostream &operator<<(std::ostream &os, const EccPub &k) {
  std::string s;
  std::ios_base::fmtflags f(os.flags());
  k._key.Save(CryptoPP::StringSink(s).Ref());
  Buffer b(s);
  os << std::hex << b;
  os.flags(f);
  return os;
}
}  // namespace crypto
}  // namespace neuro
