#ifndef NEURO_SRC_MESSAGES_CONFIG_CONFIG_HPP
#define NEURO_SRC_MESSAGES_CONFIG_CONFIG_HPP

#include <google/protobuf/message.h>
#include <google/protobuf/util/json_util.h>
#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>
#include <filesystem>
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
  explicit Config(const Path &filepath) {
    if (!messages::from_json_file(filepath.string(), this)) {
      std::string s = "Could not parse configuration file " +
                      filepath.string() + " from " +
                      boost::filesystem::current_path().native();
      throw std::runtime_error(s);
    }
  }
  explicit Config(const std::string &data) {
    if (!messages::from_json(data, this)) {
      const auto s =
          std::string{"Could not parse configuration <" + data + ">"};
      throw std::runtime_error(s);
    }
  }
};

}  // namespace config
}  // namespace messages
}  // namespace neuro

#endif /* NEURO_SRC_MESSAGES_CONFIG_CONFIG_HPP */
