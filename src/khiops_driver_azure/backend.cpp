#include "khiops_driver_common/backend.hpp"
#include "khiops_driver_common/logging.hpp"
#include "khiops_driver_azure/version.hpp"
#include <memory>
#include <spdlog/spdlog.h>

using namespace std;

namespace khiops_driver_azure {

int GetDriverName(std::string *result) {
    *result = "Azure driver";
    return 0;
}

int GetDriverVersion(std::string *result) {
    *result = DRIVER_VERSION;
    return 0;
}

int GetDriverScheme(std::string *result) {
    *result = "https";
    return 0;
}

spdlog::logger *GetLogger() {
    return khiops_driver_common::GetLogger("azdriver", "AZURE_DRIVER_LOGFILE", "AZURE_DRIVER_LOGLEVEL");
}

}

namespace khiops_driver_common {

const Backend *GetBackend() {
    static unique_ptr<Backend> backend = nullptr;
    if (backend == nullptr) {
        backend = make_unique<Backend>();
        backend->GetDriverName = &khiops_driver_azure::GetDriverName;
        backend->GetLogger = &khiops_driver_azure::GetLogger;
        backend->GetDriverVersion = &khiops_driver_azure::GetDriverVersion;
        backend->GetDriverScheme = &khiops_driver_azure::GetDriverScheme;
    }
    return backend.get();
}

}