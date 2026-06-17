#include "khiops_driver_common/logging.hpp"

namespace khiops_driver_common {

spdlog::logger *GetLogger() {
    return GetLogger("azdriver", "AZURE_DRIVER_LOGFILE", "AZURE_DRIVER_LOGLEVEL");
}

}