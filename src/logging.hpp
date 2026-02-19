#pragma once

#include "util.hpp"
#include <azure/core/diagnostics/logger.hpp>
#include <memory>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/ostream_sink.h>
#include <spdlog/sinks/stdout_sinks.h>
#include <spdlog/spdlog.h>
#include <sstream>
#include <string>
#include <vector>

namespace az {
namespace logging {
static std::string logstring = "";
static std::ostringstream logstringstream("");
static std::shared_ptr<spdlog::sinks::ostream_sink_st> stringstreamsink = nullptr;
static std::shared_ptr<spdlog::sinks::stderr_sink_st> stderrsink = nullptr;
static std::shared_ptr<spdlog::sinks::basic_file_sink_st> filesink = nullptr;
static std::vector<std::shared_ptr<spdlog::sinks::sink>> defaultloggersinks = {};
static std::shared_ptr<spdlog::logger> defaultlogger = nullptr;

class LogInitializer {
public:
    LogInitializer() {
        logstring = "";
        logstringstream = std::ostringstream("");
        stringstreamsink =
            std::make_shared<spdlog::sinks::ostream_sink_st>(logstringstream);
        stringstreamsink->set_level(spdlog::level::err);
        defaultloggersinks.push_back(stringstreamsink);

        stderrsink = std::make_shared<spdlog::sinks::stderr_sink_st>();
        stderrsink->set_level(spdlog::level::from_str(
            ::az::util::env::GetEnvVarOrDefault("AZURE_DRIVER_LOGLEVEL", "off", true)));
        defaultloggersinks.push_back(stderrsink);

        std::string sLogFileEnvVal = ::az::util::env::GetEnvVar("AZURE_DRIVER_LOGFILE", true);
        if (!sLogFileEnvVal.empty()) {
            filesink = std::make_shared<spdlog::sinks::basic_file_sink_st>(sLogFileEnvVal);
            filesink->set_level(spdlog::level::trace);
            defaultloggersinks.push_back(filesink);
        }

        defaultloggersinks = {stringstreamsink, stderrsink};

        defaultlogger = std::make_shared<spdlog::logger>(
            "default", defaultloggersinks.begin(), defaultloggersinks.end());
        defaultlogger->set_level(spdlog::level::trace); // Let the sinks choose the log level.
        spdlog::set_default_logger(defaultlogger);

        /* Disable Azure SDK logging.
           Note: This will not prevent Azure CLI, called as a subprocess by the
           Azure SDK, to log errors such as "Please run 'az login' to authenticate".
        */
        Azure::Core::Diagnostics::Logger::SetListener(
            [](Azure::Core::Diagnostics::Logger::Level, std::string const &) {});
    }

    ~LogInitializer() {
        spdlog::shutdown();
        logstring = "";
        logstringstream = std::ostringstream("");
        stringstreamsink = nullptr;
        stderrsink = nullptr;
        filesink = nullptr;
        defaultloggersinks = {};
        defaultlogger = nullptr;
    }
};
} // namespace logging
} // namespace az
