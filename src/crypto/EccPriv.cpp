#include "crypto/EccPriv.hpp"
#include <iomanip>

namespace neuro {
namespace crypto {

EccPriv::EccPriv(std::shared_ptr<CryptoPP::AutoSeededRandomPool> prng)
    : _prng(prng), _params(CryptoPP::ASN1::secp256k1()), _key() {
  _key.Initialize(*_prng, _params);

  if (!_key.Validate(*_prng, 3)) {
    throw std::runtime_error("Could not validate privated key");
  }
}

EccPriv::EccPriv(std::shared_ptr<CryptoPP::AutoSeededRandomPool> prng,
                 const std::string &filepath)
    : _prng(prng) {
  if (!load(filepath)) {
    throw std::runtime_error("Public key validation failed");
  }
}

bool EccPriv::save(const std::string &filepath) const {
  CryptoPP::FileSink fs(filepath.c_str(), true);
  _key.Save(fs);

  return true;
}

bool EccPriv::save(Buffer *buffer) const {
  std::string s;
  _key.Save(CryptoPP::StringSink(s).Ref());
  buffer->copy(s);
  return true;
}

bool EccPriv::load(const std::string &filepath) {
  CryptoPP::FileSource fs(filepath.c_str(), true);
  _key.Load(fs);
  return (_key.Validate(*_prng, 3));
}

bool EccPriv::load(const Buffer &buffer) {
  CryptoPP::StringSource array(reinterpret_cast<const byte *>(buffer.data()),
                               buffer.size(), true);
  _key.Load(array);
  return true;
}

void EccPriv::sign(const uint8_t *data, const std::size_t size,
                   uint8_t *signature) {
  CryptoPP::ECDSA<CryptoPP::ECP, CryptoPP::SHA256>::Signer signer(_key);
  signer.SignMessage(*_prng, data, size, signature);
}

Buffer EccPriv::sign(const uint8_t *data, const std::size_t size) {
  const auto siglen = sign_length();
  Buffer signature(siglen, 0);
  sign(data, size, signature.data());

  return signature;
}

Buffer EccPriv::sign(const Buffer &input) {
  return sign(input.data(), input.size());
}

EccPub EccPriv::make_public_key() {
  EccPub::Key pub_key;
  _key.MakePublicKey(pub_key);
  return EccPub{pub_key};
}

bool EccPriv::operator==(const EccPriv &key) const {
  CryptoPP::ByteQueue queue0;
  CryptoPP::ByteQueue queue1;
  _key.Save(queue0);
  key._key.Save(queue1);
  return (queue0 == queue1);
}

std::ostream &operator<<(std::ostream &os, const EccPriv &priv) {
  std::ios_base::fmtflags f(os.flags());

  os << std::hex << priv._key.GetPrivateExponent();
  os.flags(f);
  return os;
}

}  // namespace crypto
}  // namespace neuro
