#ifndef NEURO_SRC_MESSAGES_MESSAGE_HPP
#define NEURO_SRC_MESSAGES_MESSAGE_HPP

#include <google/protobuf/message.h>
#include <google/protobuf/util/json_util.h>
#include <string>
#include <fstream>
#include "common/types.hpp"
#include "messages.pb.h"
#include "mongo/mongo.hpp"

namespace neuro {
namespace messages {

using Packet = google::protobuf::Message;
using Type = Body::BodyCase;
  
Type get_type(const Body &body) {
  return body.body_case();
}

bool from_buffer(const Buffer &buffer,
                 Packet *packet) {
  return packet->ParseFromArray(buffer.data(), buffer.size());
}
  
bool from_json(const std::string &json,
               Packet *packet) {
  google::protobuf::util::JsonParseOptions options;
  auto r = google::protobuf::util::JsonStringToMessage(json, packet, options);
  return r.ok();
}

bool from_json_file(const std::string &path,
                    Packet *packet) {

  std::ifstream t(path);
  std::string str((std::istreambuf_iterator<char>(t)),
                  std::istreambuf_iterator<char>());
  return from_json(str, packet);
}

bool from_bson(const bsoncxx::document::value &doc,
               Packet *packet) {
  const auto mongo_json = bsoncxx::to_json(doc);
  return from_json(mongo_json, packet);
}

void to_buffer(const Packet &packet, Buffer *buffer) {
  packet.SerializeToArray(buffer->data(), buffer->size());
}
  
void to_json(const Packet &packet, std::string *output) {
  google::protobuf::util::MessageToJsonString(packet, output);
}
  
void to_bson(const Packet &packet, bsoncxx::document::value *bson) {
  std::string json;
  to_json(packet, &json);
  *bson = bsoncxx::from_json(json);
}

std::ostream & operator<< (std::ostream &os, const Packet &packet);
  
}  // messages
}  // neuro

#endif /* NEURO_SRC_MESSAGES_MESSAGE_HPP */
