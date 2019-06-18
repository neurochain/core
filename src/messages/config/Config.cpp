#include "messages/config/Config.hpp"
#include "messages/Message.hpp"

namespace neuro {
namespace messages {
namespace config {

Config::Config(const Path &filepath) {
  if (!messages::from_json_file(filepath.string(), this)) {
    std::string s = "Could not parse configuration file " + filepath.string() +
                    " from " + boost::filesystem::current_path().native();
    throw std::runtime_error(s);
  }
}

Config::Config(const std::string &data) {
  if (!messages::from_json(data, this)) {
    const auto s = std::string{"Could not parse configuration <" + data + ">"};
    throw std::runtime_error(s);
  }
}

}  // namespace config
}  // namespace messages
}  // namespace neuro
