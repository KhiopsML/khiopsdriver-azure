#include "logging.hpp"
#include "util.hpp"
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/ostream_sink.h>
#include <spdlog/sinks/stdout_sinks.h>
#include <sstream>
#include <string>
#include <vector>

using namespace std;

namespace az {
namespace logging {
namespace {

string logstring;
ostringstream logstringstream;
shared_ptr<spdlog::sinks::ostream_sink_st> stringstreamsink;
shared_ptr<spdlog::sinks::stderr_sink_st> stderrsink;
shared_ptr<spdlog::sinks::basic_file_sink_st> filesink;
vector<shared_ptr<spdlog::sinks::sink>> sinks;
shared_ptr<spdlog::logger> logger;

// Logging lazy initializer
class LogInitializer {
public:
  LogInitializer() {
    spdlog::level::level_enum loglevel =
        spdlog::level::from_str(::az::util::env::GetEnvVarOrDefault(
            "AZURE_DRIVER_LOGLEVEL", "off", true));

    logstring.clear();
    logstringstream = ostringstream("");
    stringstreamsink =
        make_shared<spdlog::sinks::ostream_sink_st>(logstringstream);
    stringstreamsink->set_level(spdlog::level::err);
    sinks.push_back(stringstreamsink);

    stderrsink = make_shared<spdlog::sinks::stderr_sink_st>();
    stderrsink->set_level(loglevel);
    sinks.push_back(stderrsink);

    string sLogFileEnvVal =
        ::az::util::env::GetEnvVar("AZURE_DRIVER_LOGFILE", true);
    if (!sLogFileEnvVal.empty()) {
      filesink = make_shared<spdlog::sinks::basic_file_sink_st>(sLogFileEnvVal);
      filesink->set_level(loglevel);
      sinks.push_back(filesink);
    }

    logger =
        make_shared<spdlog::logger>("azdriver", sinks.begin(), sinks.end());
    logger->set_level(
        spdlog::level::trace); // Let the sinks choose the log level.
    spdlog::register_logger(logger);
  }

  ~LogInitializer() {
    logstring.clear();
    logstringstream = ostringstream("");
    stringstreamsink.reset();
    stderrsink.reset();
    filesink.reset();
    sinks.clear();
    logger.reset();
  }
};

} // namespace

const shared_ptr<spdlog::logger> &getLogger() {
  static LogInitializer logInitializer;
  return logger;
}

const string &getLastError() {
  logstring = logstringstream.str();
  logstringstream.str("");
  logstringstream.clear();
  return logstring;
}

} // namespace logging
} // namespace az
