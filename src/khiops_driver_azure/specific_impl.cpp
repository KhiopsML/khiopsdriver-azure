#include "khiops_driver_common/logging.hpp"

namespace khiops_driver_common {

spdlog::logger *GetLogger() {
    return GetLogger("azuredriver", "AZURE_DRIVER_LOGFILE", "AZURE_DRIVER_LOGLEVEL");
}

}