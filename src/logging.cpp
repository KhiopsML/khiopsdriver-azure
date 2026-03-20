#include "logging.hpp"
#include "khiops_driver_common/logging.hpp"
#include "khiops_driver_common/util.hpp"
#include <memory>
#include <spdlog/spdlog.h>
#include <string>

using namespace std;

namespace az {
namespace logging {

static string loglevel;
static string logfile;
static shared_ptr<spdlog::logger> logger;
namespace {
struct LazyLoggerInitializer {
  LazyLoggerInitializer() {
    loglevel = khiops_driver_common::util::env::GetEnvVarOrDefault(
        "AZURE_DRIVER_LOGLEVEL", "off", true);
    logfile = khiops_driver_common::util::env::GetEnvVar("AZURE_DRIVER_LOGFILE",
                                                         true);
    logger = khiops_driver_common::logging::getLogger("azdriver", loglevel,
                                                      logfile, false);
  }
};
} // anonymous namespace

const shared_ptr<spdlog::logger> &getLogger() {
  static LazyLoggerInitializer _;
  return logger;
}

} // namespace logging
} // namespace az