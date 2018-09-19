#ifndef NEURO_SRC_LOGGER_HPP
#define NEURO_SRC_LOGGER_HPP

/****************************************************************
 * To use the logger in any file:
 * #include "common/logger.hpp"
 *
 * LOG_TRACE << "Trace message";
 * LOG_DEBUG << "Debug message";
 * LOG_INFO << "Info message";
 * LOG_WARNING << "Warning message";
 * LOG_ERROR << "Error message";
 * LOG_FATAL << "Fatal message";
 *
 *****************************************************************/

#include <boost/core/null_deleter.hpp>
#include <boost/log/core.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/expressions/formatters/date_time.hpp>
#include <boost/log/sinks/sync_frontend.hpp>
#include <boost/log/sinks/text_file_backend.hpp>
#include <boost/log/sinks/text_ostream_backend.hpp>
#include <boost/log/sources/global_logger_storage.hpp>
#include <boost/log/support/date_time.hpp>
#include <boost/log/trivial.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>
#include <boost/make_shared.hpp>
#include <boost/shared_ptr.hpp>
#include <fstream>
#include <ostream>

#include "messages/Message.hpp"
#include "messages.pb.h"
#include "config.pb.h"


// register a global logger
BOOST_LOG_GLOBAL_LOGGER(logger, boost::log::sources::severity_logger_mt<
                                    boost::log::trivial::severity_level>)

BOOST_LOG_ATTRIBUTE_KEYWORD(timestamp, "TimeStamp", boost::posix_time::ptime)

namespace logging = boost::log;
namespace src = boost::log::sources;
namespace sinks = boost::log::sinks;
namespace expr = boost::log::expressions;
namespace keywords = boost::log::keywords;
namespace attrs = boost::log::attributes;


namespace neuro {
namespace log {


using text_sink = sinks::synchronous_sink<sinks::text_ostream_backend>;
using file_sink = sinks::synchronous_sink<sinks::text_file_backend>;


logging::formatter neuro_formatter();
void add_file_sink(const std::string &log_file, const size_t rotation_size_mb);
void add_stdout_sink();
/*
  Possibles values for log.severity (from config file):
   "trace",
   "debug",
   "info",
   "warning",
   "error",
   "fatal"
*/
void from_config(const messages::config::Logs &logs_config);

}  // namespace log
}  // namespace neuro

#define LOG_TRACE BOOST_LOG_SEV(logger::get(), boost::log::trivial::trace)
#define LOG_DEBUG BOOST_LOG_SEV(logger::get(), boost::log::trivial::debug)
#define LOG_INFO BOOST_LOG_SEV(logger::get(), boost::log::trivial::info)
#define LOG_WARNING BOOST_LOG_SEV(logger::get(), boost::log::trivial::warning)
#define LOG_ERROR BOOST_LOG_SEV(logger::get(), boost::log::trivial::error)
#define LOG_FATAL BOOST_LOG_SEV(logger::get(), boost::log::trivial::fatal)

#endif
