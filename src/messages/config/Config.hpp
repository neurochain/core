#ifndef NEURO_SRC_MESSAGES_CONFIG_CONFIG_HPP
#define NEURO_SRC_MESSAGES_CONFIG_CONFIG_HPP

#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>
#include <google/protobuf/message.h>
#include <google/protobuf/util/json_util.h>
#include <fstream>
#include <stdexcept>
#include <string>

#include "common/types.hpp"
#include "config.pb.h"
#include "crypto/KeyPriv.hpp"

namespace neuro {
namespace messages {
namespace config {

class Config : public _Config {
 public:
  Config() {}
  explicit Config(const Path &filepath);
  explicit Config(const std::string &data);
};

}  // namespace config
}  // namespace messages
}  // namespace neuro

#endif /* NEURO_SRC_MESSAGES_CONFIG_CONFIG_HPP */
