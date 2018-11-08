#ifndef NEURO_SRC_MESSAGES_CONFIG_CONFIG_HPP
#define NEURO_SRC_MESSAGES_CONFIG_CONFIG_HPP

#include <google/protobuf/message.h>
#include <google/protobuf/util/json_util.h>
#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>
#include <fstream>
#include <stdexcept>
#include <string>
#include "common/types.hpp"
#include "config.pb.h"
#include "crypto/EccPriv.hpp"
#include "messages/Message.hpp"

namespace neuro {
namespace messages {
namespace config {

class Config : public _Config {
 public:
  Config() {}
  Config(const std::string &filepath) {
    if (!messages::from_json_file(filepath, this)) {
      std::string s = "Coult not parse configuration file " + filepath +
                      " from " + boost::filesystem::current_path().native();
      throw std::runtime_error(s);
    }
  }
};

}  // namespace config
}  // namespace messages
}  // namespace neuro

#endif /* NEURO_SRC_MESSAGES_CONFIG_CONFIG_HPP */
