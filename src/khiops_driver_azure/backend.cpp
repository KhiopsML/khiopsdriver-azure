#include "khiops_driver_common/backend.hpp"
#include "khiops_driver_common/logging.hpp"
#include "khiops_driver_common/util.hpp"
#include "khiops_driver_azure/util.hpp"
#include "khiops_driver_azure/version.hpp"
#include "khiops_driver_azure/servicerequest.hpp"
#include <memory>
#include <spdlog/spdlog.h>
#include <azure/core/diagnostics/logger.hpp>

using namespace std;
using namespace khiops_driver_azure;

namespace khiops_driver_common {

spdlog::logger *GetLogger() {
    return GetLogger("azdriver", "AZURE_DRIVER_LOGFILE", "AZURE_DRIVER_LOGLEVEL");
}

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

int IsReadOnly(bool *result) {
    *result = false;
    return 0;
}

int Initialize() {
    // Disable Azure SDK logging.
    // Note: This will not prevent Azure CLI, called as a subprocess by the
    // Azure SDK, to log errors such as "Please run 'az login' to authenticate".
    Azure::Core::Diagnostics::Logger::SetListener([](Azure::Core::Diagnostics::Logger::Level, string const &) {});
    return 0;
}

int Finalize() {
    // Nothing to do.
    return 0;
}

int GetSystemPreferredBufferSize(size_t *result) {
    constexpr size_t DEFAULT_PREFERRED_BUFFER_SIZE = 4ULL * 1024ULL * 1024ULL;
    const string ENVIRONMENT_VARIABLE_NAME = "AZURE_PREFERRED_BUFFER_SIZE";
    string environment_variable_preferred_buffer_size = util::env::GetEnvVar(ENVIRONMENT_VARIABLE_NAME);
    if (!environment_variable_preferred_buffer_size.empty()) {
        try {
            *result = stoull(environment_variable_preferred_buffer_size);
            return 0;
        } catch (const invalid_argument &) {
            GetLogger()->warn(
                "Value {} of environment variable {} is not a valid number. Falling back to default {}...",
                environment_variable_preferred_buffer_size, ENVIRONMENT_VARIABLE_NAME, DEFAULT_PREFERRED_BUFFER_SIZE
            );
        } catch (const out_of_range &) {
            GetLogger()->warn(
                "Value {} of environment variable {} is out of range. Falling back to default {}...",
                environment_variable_preferred_buffer_size, ENVIRONMENT_VARIABLE_NAME, DEFAULT_PREFERRED_BUFFER_SIZE
            );
        }
    }
    *result = DEFAULT_PREFERRED_BUFFER_SIZE;
    return 0;
}

int FileExists(bool *result, const std::string &sFilePathName) {
    ServiceRequest request;
    if (ParseUrl(&request, sFilePathName)) {
        return -1;
    }
    if (request.storageType == BLOB) {
        if (request.bDir) {
            *result = true;  // there is no such concept as a directory when dealing with blob services
        } else {
            *result = !ListBlobs(request).empty();
        }
    } else  // SHARE
    {
        if (request.bDir) {
            *result = !ListDirs(request).empty();
        } else {
            *result = !ListFiles(request).empty();
        }
    }
    return 0;
}

int DirExists(bool *result, const std::string &sFilePathName) {
    GetLogger()->error("Not implemented!");
    return -1;
}

int GetFileSize(size_t *result, const std::string &filename) {
    GetLogger()->error("Not implemented!");
    return -1;
}

int FOpen(khiops_driver_common::FileStream &stream, const std::string &filename) {
    GetLogger()->error("Not implemented!");
    return -1;
}

int FClose(const khiops_driver_common::FileStream &stream) {
    GetLogger()->error("Not implemented!");
    return -1;
}

int FRead(size_t *result, void *ptr, size_t size, size_t count, khiops_driver_common::FileStream &stream) {
    GetLogger()->error("Not implemented!");
    return -1;
}

int FSeek(khiops_driver_common::FileStream &stream, long long int offset, int whence) {
    GetLogger()->error("Not implemented!");
    return -1;
}

int FWrite(size_t *result, const void *ptr, size_t size, size_t count, const khiops_driver_common::FileStream &stream) {
    GetLogger()->error("Not implemented!");
    return -1;
}

int FFlush(const khiops_driver_common::FileStream &stream) {
    GetLogger()->error("Not implemented!");
    return -1;
}

int Remove(const std::string &filename) {
    GetLogger()->error("Not implemented!");
    return -1;
}

int Mkdir(const std::string &pathname) {
    GetLogger()->error("Not implemented!");
    return -1;
}

int Rmdir(const std::string &pathname) {
    GetLogger()->error("Not implemented!");
    return -1;
}

int DiskFreeSpace(size_t *result, const std::string &filename) {
    GetLogger()->error("Not implemented!");
    return -1;
}

int CopyToLocal(const std::string &sourcefilename, const std::string &destfilename) {
    GetLogger()->error("Not implemented!");
    return -1;
}

int CopyFromLocal(const std::string &sourcefilename, const std::string &destfilename) {
    GetLogger()->error("Not implemented!");
    return -1;
}

int Concat(const std::string &destfilename, const std::vector<std::string> &sourcefilenames) {
    GetLogger()->error("Not implemented!");
    return -1;
}

int ComposeMultifile(const std::string &sDestFilePathName, const std::vector<std::string> &sSourceFilePathNames) {
    GetLogger()->error("Not implemented!");
    return -1;
}

}