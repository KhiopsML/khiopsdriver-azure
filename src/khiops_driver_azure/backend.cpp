#include "khiops_driver_common/backend.hpp"
#include "khiops_driver_common/logging.hpp"
#include "khiops_driver_azure/version.hpp"
#include <memory>
#include <spdlog/spdlog.h>

using namespace std;

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
    GetLogger()->error("Not implemented!");
    return -1;
}

int Finalize() {
    GetLogger()->error("Not implemented!");
    return -1;
}

int GetSystemPreferredBufferSize(size_t *result) {
    GetLogger()->error("Not implemented!");
    return -1;
}

int FileExists(bool *result, const std::string &sFilePathName) {
    GetLogger()->error("Not implemented!");
    return -1;
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