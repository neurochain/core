#include "crypto/EccPub.hpp"
#include <cryptopp/hex.h>
#include <iomanip>
#include <iostream>
#include "common/logger.hpp"
#include "messages/Message.hpp"

namespace neuro {
namespace crypto {

EccPub::EccPub(const std::string &filepath) { load(filepath); }

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

bool EccPub::load(const uint8_t *data, const std::size_t size) {
  CryptoPP::StringSource array(reinterpret_cast<const byte *>(data), size,
                               true);
  _key.Load(array);
  return true;
}

bool EccPub::load(const messages::KeyPub &keypub) {
  CryptoPP::ECP::Point point;
  if (keypub.has_raw_data()) {
    // Compressed public key see
    // https://www.cryptopp.com/wiki/Elliptic_Curve_Digital_Signature_Algorithm
    LOG_TRACE;
    // const auto &raw_data = keypub.raw_data();
    const auto &raw_data = '\x03' + keypub.raw_data();
    LOG_TRACE << "RAW KEY" << keypub;
    LOG_DEBUG << "RAW DATA SIZE " << keypub.raw_data().size();
    LOG_DEBUG << "RAW DATA SIZE " << raw_data.size();

    std::stringstream ss;
    ss << std::hex << raw_data;
    std::string encoded = ss.str();

    CryptoPP::HexEncoder encoder;
    auto decoded = reinterpret_cast<const byte *>(raw_data.data());
    encoder.Put(decoded, sizeof(decoded));
    encoder.MessageEnd();

    CryptoPP::word64 size = encoder.MaxRetrievable();

    LOG_DEBUG << "SIZE " << size;
    if (size) {
      encoded.resize(size);
      encoder.Get((byte *)&encoded[0], encoded.size());
    }

    LOG_DEBUG << "HEX DATA " << encoded;

    // CryptoPP::StringSource ss(raw_data, true);

    CryptoPP::HexDecoder decoder;
    LOG_TRACE;
    CryptoPP::StringSource source(encoded, true, &decoder);
    LOG_TRACE;
    _key.GetGroupParameters().GetCurve().DecodePoint(point, source,
                                                     source.MaxRetrievable());
    LOG_TRACE << std::endl;
  } else if (keypub.has_hex_data()) {
    // Compressed public key
    LOG_TRACE;
    const auto &hex_data = "03" + keypub.hex_data();
    LOG_TRACE;

    CryptoPP::HexDecoder decoder;
    LOG_TRACE;
    CryptoPP::StringSource ss(hex_data, true, &decoder);
    LOG_TRACE;
    _key.GetGroupParameters().GetCurve().DecodePoint(point, ss,
                                                     ss.MaxRetrievable());
    LOG_TRACE;
  } else {
    // not possible since we have a oneof
    return false;
  }
  _key.Initialize(CryptoPP::ASN1::secp256r1(), point);

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
  const auto x = _key.GetPublicElement().x;
  const auto size = x.MinEncodedSize();
  byte output[size];
  x.Encode(output, size);
  key_pub->set_raw_data(
      std::string(reinterpret_cast<char const *>(output), size));
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
