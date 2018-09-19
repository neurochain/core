#ifndef NEURO_SRC_MESSAGES_MESSAGE_HPP
#define NEURO_SRC_MESSAGES_MESSAGE_HPP

#include <google/protobuf/message.h>
#include <google/protobuf/util/json_util.h>
#include <string>
#include <fstream>
#include "messages.pb.h"
#include "mongo/mongo.hpp"

namespace neuro {
namespace messages {

using Packet = google::protobuf::Message;
 
Type get_type(const Body &body) {

  if (body.has_hello()) {
    return HELLO; 
  } else if (body.has_world()) {
    return WORLD; 
  } else if (body.has_get_peers()) {
    return GET_PEERS; 
  } else if (body.has_peers()) {
    return PEERS; 
  } else if (body.has_get_roots()) {
    return GET_ROOTS; 
  } else if (body.has_roots()) {
    return ROOTS; 
  } else if (body.has_transaction()) {
    return TRANSACTION; 
  } else if (body.has_get_block()) {
    return GET_BLOCK; 
  } else if (body.has_block()) {
    return BLOCK;
  } 

  return UNKNOWN;
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

std::string to_json(const Packet &packet) {
  std::string output;
  google::protobuf::util::MessageToJsonString(packet, &output);
  return output;
}
  
bsoncxx::document::value to_bson(const Packet &packet) {
  const auto json = to_json(packet);
  return bsoncxx::from_json(json);
}

}  // messages
}  // neuro

#endif /* NEURO_SRC_MESSAGES_MESSAGE_HPP */
