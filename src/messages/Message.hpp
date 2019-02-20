#ifndef NEURO_SRC_MESSAGES_MESSAGE_HPP
#define NEURO_SRC_MESSAGES_MESSAGE_HPP

#include <google/protobuf/message.h>
#include <google/protobuf/util/json_util.h>
#include <fstream>
#include <functional>
#include <string>
#include <type_traits>

#include "common/Buffer.hpp"
#include "common/logger.hpp"
#include "common/types.hpp"
#include "crypto/EccPriv.hpp"
#include "ledger/mongo.hpp"
#include "messages.pb.h"
#include "messages/Peer.hpp"

namespace neuro {
namespace messages {

using BlockHeight = decltype(((BlockHeader *)nullptr)->height());
using BlockID = decltype(((BlockHeader *)nullptr)->id());
using TransactionID = decltype(((Transaction *)nullptr)->id());
using Packet = google::protobuf::Message;

using Type = Body::BodyCase;
using BranchID = uint32_t;
using BlockScore = double;

Type get_type(const Body &body);

bool from_buffer(const Buffer &buffer, Packet *packet);
bool from_json(const std::string &json, Packet *packet);
bool from_json_file(const std::string &path, Packet *packet);
bool from_bson(const bsoncxx::document::value &doc, Packet *packet);
bool from_bson(const bsoncxx::document::view &doc, Packet *packet);

std::size_t to_buffer(const Packet &packet, Buffer *buffer);
void to_json(const Packet &packet, std::string *output);
std::string to_json(const Packet &packet);
bsoncxx::document::value to_bson(const Packet &packet);
std::ostream &operator<<(std::ostream &os, const Packet &packet);

template <typename T>
std::ostream &operator<<(
    std::ostream &os, const ::google::protobuf::RepeatedPtrField<T> &packets) {
  for (const auto &packet : packets) {
    os << packet << std::endl;
  }
  return os;
}

// struct PacketHash {
//   template <typename T>
//   const typename std::enable_if<std::is_base_of<Packet, T>::value,
//   std::size_t>::type operator()(const T &packet) const noexcept {
//     neuro::Buffer buffer;
//     neuro::messages::to_buffer(packet, &buffer);
//     const auto hash = std::hash<neuro::Buffer>{}(buffer);
//     return hash;
//     }
// };

// bool operator==(const messages::Peer &a, const messages::Peer &b);

void set_transaction_hash(Transaction *transaction);
void set_block_hash(Block *block);
int32_t fill_header(messages::Header *header);
int32_t fill_header_reply(const messages::Header &header_request,
                          messages::Header *header_reply);

class Message : public _Message {
 public:
  Message() { fill_header(mutable_header()); }
  Message(const std::string &json) { from_json(json, this); }

  Message(const Path &path) { from_json_file(path.string(), this); }
  virtual ~Message() {}
};

class NCCAmount : public _NCCAmount {
 public:
  NCCAmount() {}
  NCCAmount(const _NCCAmount &nccsdf) : _NCCAmount(nccsdf) {}
  NCCAmount(uint64_t amount) { set_value(amount); }
};

template <typename TA>
typename std::enable_if<
    std::is_base_of<::neuro::messages::Packet, TA>::value,
    bool>::type
operator==(const TA &a, const TA &b) {
  std::string json_a, json_b;
  to_json(a, &json_a);
  to_json(b, &json_b);
  bool res = json_a == json_b;

  return res;
}
  
}  // namespace messages  
}  // namespace neuro

template <typename TA, typename TB>
typename std::enable_if<
    std::is_base_of<::neuro::messages::Packet, TA>::value &&
        std::is_base_of<::neuro::messages::Packet, TB>::value,
    bool>::type
operator==(const TA &a, const TB &b) {
  std::string json_a, json_b;
  to_json(a, &json_a);
  to_json(b, &json_b);
  bool res = json_a == json_b;

  return res;
}

template <typename T>
struct PacketHash {
  std::size_t operator()(typename std::enable_if<std::is_base_of<::neuro::messages::Packet, T>::value, T>::type  const& s) const noexcept {
    return 42;
  }
};


// template <>
// struct hash<neuro::::neuro::messages::KeyPub> {
//   using argument_type = neuro::::neuro::messages::KeyPub;
//   using result_type = size_t;

//   result_type operator()(argument_type const &packet) const noexcept {
//     neuro::Buffer buffer;
//     neuro::::neuro::messages::to_buffer(packet, &buffer);
//     const auto hash = std::hash<neuro::Buffer>{}(buffer);
//     return hash;
//   }
// };

// namespace std {
// template <typename T>
// struct hash <typename std::enable_if<
// 	       std::is_base_of<::neuro::messages::Packet, T>::value, T>::type> {
//   using result_type = std::size_t;

//   result_type operator()(argument_type const &packet) const noexcept {
//     neuro::Buffer buffer;
//     neuro::messages::to_buffer(packet, &buffer);
//     const auto hash = std::hash<neuro::Buffer>{}(buffer);
//     return hash;
//   }
// };

// namespace std {
// template <class T>
// struct hash  {

//   typename std::enable_if<
//     std::is_base_of<::neuro::messages::Packet, T>::value, T>::type operator()(argument_type const &packet) const noexcept {
//     neuro::Buffer buffer;
//     neuro::messages::to_buffer(packet, &buffer);
//     const auto hash = std::hash<neuro::Buffer>{}(buffer);
//     return hash;
//   }
// };


//}  // namespace std

//}  // namespace std

#endif /* NEURO_SRC_MESSAGES_MESSAGE_HPP */
