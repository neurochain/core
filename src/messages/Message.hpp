#ifndef NEURO_SRC_MESSAGES_MESSAGE_HPP
#define NEURO_CORE_SRC_MESSAGES_MESSAGE_HPP

#include "common/types.hpp"
#include "messages.pb.h"
#include "mongo/mongo.hpp"
#include <fstream>
#include <google/protobuf/message.h>
#include <google/protobuf/util/json_util.h>
#include <string>

namespace neuro {
namespace messages {

using Packet = google::protobuf::Message;
using Type = Body::BodyCase;

Type get_type(const Body &body);

bool from_buffer(const Buffer &buffer, Packet *packet);
bool from_json(const std::string &json, Packet *packet);
bool from_json_file(const std::string &path, Packet *packet);

bool from_bson(const bsoncxx::document::value &doc, Packet *packet);
void to_buffer(const Packet &packet, Buffer *buffer);
void to_json(const Packet &packet, std::string *output);
void to_bson(const Packet &packet, bsoncxx::document::value *bson);
std::ostream &operator<<(std::ostream &os, const Packet &packet);

} // namespace messages
} // namespace neuro

#endif /* NEURO_SRC_MESSAGES_MESSAGE_HPP */
