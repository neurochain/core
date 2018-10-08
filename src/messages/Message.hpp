#ifndef NEURO_SRC_MESSAGES_MESSAGE_HPP
#define NEURO_SRC_MESSAGES_MESSAGE_HPP

#include <google/protobuf/message.h>
#include <google/protobuf/util/json_util.h>
#include <string>
#include <fstream>
#include "common/types.hpp"
#include "messages.pb.h"
#include "mongo/mongo.hpp"
#include "messages/Hasher.hpp"
#include "crypto/EccPriv.hpp"

namespace neuro {
namespace messages {


using BlockHeight = decltype(((BlockHeader*)nullptr)->height());
using BlockID = decltype(((BlockHeader*)nullptr)->id());
using TransactionID = decltype(((Transaction*)nullptr)->id());
using Packet = google::protobuf::Message;
using Type = Body::BodyCase;
using Address = Hasher;
  
Type get_type(const Body &body);

bool from_buffer(const Buffer &buffer, Packet *packet);
bool from_json(const std::string &json, Packet *packet);
bool from_json_file(const std::string &path, Packet *packet);
bool from_bson(const bsoncxx::document::value &doc, Packet *packet);
bool from_bson(const bsoncxx::document::view &doc, Packet *packet);

std::size_t  to_buffer(const Packet &packet, Buffer *buffer);  
void to_json(const Packet &packet, std::string *output);
bsoncxx::document::value to_bson(const Packet &packet);
std::ostream & operator<< (std::ostream &os, const Packet &packet);
  
}  // messages
}  // neuro

#endif /* NEURO_SRC_MESSAGES_MESSAGE_HPP */
