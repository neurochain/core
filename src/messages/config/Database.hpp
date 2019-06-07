#ifndef NEURO_SRC_MESSAGES_CONFIG_DATABASE_HPP
#define NEURO_SRC_MESSAGES_CONFIG_DATABASE_HPP

#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>

namespace neuro {
namespace messages {
namespace config {

class Database : public _Database {
 public:
  Database() {}
  explicit Database(const Path &filepath) {
    if (!messages::from_json_file(filepath.string(), this)) {
      std::string s = "Could not parse configuration file " +
                      filepath.string() + " from " +
                      boost::filesystem::current_path().native();
      throw std::runtime_error(s);
    }
  }

  explicit Database(const std::string &data) {
    if (!messages::from_json(data, this)) {
      throw std::runtime_error(
          std::string{"Could not parse configuration " + data});
    }
  }

  Database(const _Database &raw) : _Database::_Database(raw) {}
};

}  // namespace config
}  // namespace messages
}  // namespace neuro

#endif /* NEURO_SRC_MESSAGES_CONFIG_DATABASE_HPP */
