#include "common/logger.hpp"
#include "config.pb.h"

BOOST_LOG_GLOBAL_LOGGER_INIT(logger, src::severity_logger_mt) {
  src::severity_logger_mt<boost::log::trivial::severity_level> logger;
  // add global attributes
  logger.add_attribute("TimeStamp", attrs::local_clock());
  return logger;
}

namespace neuro {
namespace log {

bool _STDOUT_ADDED{false};

logging::formatter neuro_formatter() {
  return expr::stream << expr::format_date_time(timestamp,
                                                "%Y-%m-%d %H:%M:%S.%f")
                      << " "
                      << "[" << logging::trivial::severity << "]"
                      << " - " << expr::smessage;
}

void add_file_sink(const std::string &log_file, const size_t rotation_size_mb) {
  auto backend = boost::make_shared<sinks::text_file_backend>(
      // file name pattern
      keywords::file_name = log_file,
      // rotate the file upon reaching `rotation_size_mb` MiB size...
      keywords::rotation_size = rotation_size_mb * 1024 * 1024);

  auto sink = boost::make_shared<file_sink>(backend);
  // boost::shared_ptr<file_sink> sink(new file_sink(backend));
  sink->set_formatter(neuro_formatter());
  sink->locked_backend()->auto_flush(true);
  logging::core::get()->add_sink(sink);
}

void add_stdout_sink() {
  if (_STDOUT_ADDED) {
    return;
  }

  auto sink = boost::make_shared<text_sink>();
  sink->locked_backend()->add_stream(
      boost::shared_ptr<std::ostream>(&std::clog, boost::null_deleter()));

  sink->set_formatter(neuro_formatter());
  sink->locked_backend()->auto_flush(true);
  logging::core::get()->add_sink(sink);

  _STDOUT_ADDED = true;
}

void from_config(const messages::config::Logs &logs) {
  

  if (!logs.has_to_stdout() || logs.to_stdout() == false) {
    add_file_sink(logs.file_path(), logs.rotation_size());
  } else {
    add_stdout_sink();
  }

  switch (logs.severity()) {
    case messages::config::Logs::trace:
      logging::core::get()->set_filter(logging::trivial::severity >=
                                       logging::trivial::trace);
      break;
    case messages::config::Logs::debug:
      logging::core::get()->set_filter(logging::trivial::severity >=
                                       logging::trivial::debug);
      break;
    case messages::config::Logs::info:
      logging::core::get()->set_filter(logging::trivial::severity >=
                                       logging::trivial::info);
      break;
    case messages::config::Logs::warning:
      logging::core::get()->set_filter(logging::trivial::severity >=
                                       logging::trivial::warning);
      break;
    case messages::config::Logs::error:
      logging::core::get()->set_filter(logging::trivial::severity >=
                                       logging::trivial::error);
      break;
    case messages::config::Logs::fatal:
      logging::core::get()->set_filter(logging::trivial::severity >=
                                       logging::trivial::fatal);
      break;
  }
}

}  // namespace log
}  // namespace neuro
