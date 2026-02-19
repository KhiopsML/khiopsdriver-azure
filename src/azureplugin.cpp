/* This is the main file that implements the functions exposed by the driver as
   a library. It delegates most of the work to the Driver class. This library
   must be C-compatible so it provides a C interface. This means that all
   high-level types taken as arguments or returned by the driver are converted,
   in this file, to basic C types. */

#ifdef __CYGWIN__
#define _CRT_SECURE_NO_WARNINGS
#endif

#include "azureplugin.hpp"
#include "driver.hpp"
#include "logging.hpp"
#include "returnval.hpp"
#include "util.hpp"
#include <memory>
#include <spdlog/spdlog.h>

#define STRINGIFY(s) #s // Use to log function argument names.

using namespace std;
using namespace az;
using az::logging::getLogger;

static unique_ptr<Driver> driver = nullptr;
static bool IsConnected() { return driver != nullptr; }

static const char *ERR_EXC_RAISED = "An exception has been raised.";
static const char *ERR_NULL_ARG =
    "Error calling '{}': passing null pointer as argument '{}'.";
static const char *ERR_INVALID_FSTREAM_MODE =
    "Tried to open file '{}' with invalid mode '{}'.";
static const char *ERR_INVALID_SEEK_ORIGIN =
    "Tried to seek from invalid origin '{}'.";

const char *driver_getDriverName() {
  try {
    getLogger()->info("Retrieving driver name...");
    return "Azure driver";
  } catch (...) {
    getLogger()->error(ERR_EXC_RAISED);
  }
  return nullptr;
}

const char *driver_getVersion() {
  try {
    getLogger()->info("Retrieving driver version...");
    return DRIVER_VERSION;
  } catch (...) {
    getLogger()->error(ERR_EXC_RAISED);
  }
  return nullptr;
}

const char *driver_getScheme() {
  try {
    getLogger()->info("Retrieving driver scheme...");
    return "https";
  } catch (...) {
    getLogger()->error(ERR_EXC_RAISED);
  }
  return nullptr;
}

int driver_isReadOnly() {
  try {
    getLogger()->info("Retrieving read-only state...");
    return nFalse;
  } catch (...) {
    getLogger()->error(ERR_EXC_RAISED);
  }
  return nGenericFailure;
}

int driver_connect() {
  try {
    getLogger()->info("Connecting...");
    driver = make_unique<Driver>();
    return nSuccess;
  } catch (...) {
    getLogger()->error(ERR_EXC_RAISED);
  }
  return nFailure;
}

int driver_disconnect() {
  try {
    getLogger()->info("Disconnecting...");
    if (!IsConnected()) {
      getLogger()->error("Cannot disconnect when already disconnected.");
      return nFailure;
    }
    driver = nullptr;
    return nSuccess;
  } catch (...) {
    getLogger()->error(ERR_EXC_RAISED);
  }
  return nFailure;
}

int driver_isConnected() {
  try {
    getLogger()->info("Retrieving connection state...");
    return IsConnected() ? nTrue : nFalse;
  } catch (...) {
    getLogger()->error(ERR_EXC_RAISED);
  }
  return nGenericFailure;
}

long long int driver_getSystemPreferredBufferSize() {
  try {
    getLogger()->info("Retrieving preferred buffer size...");
    if (!IsConnected()) {
      getLogger()->error("Cannot retrieve preferred buffer size when disconnected.");
      return nGenericFailure;
    }
    return driver->GetPreferredBufferSize();
  } catch (...) {
    getLogger()->error(ERR_EXC_RAISED);
  }
  return nGenericFailure;
}

int driver_fileExists(const char *sUrl) {
  try {
    getLogger()->info("Checking if file exists at URL {}...", sUrl);
    if (!IsConnected()) {
      getLogger()->error("Cannot check if file exists when disconnected.");
      return nGenericFailure;
    }
    if (!sUrl) {
      getLogger()->error(ERR_NULL_ARG, __func__, STRINGIFY(sUrl));
      return nGenericFailure;
    }
    bool result;
    if (driver->Exists(&result, sUrl)) {
      return nGenericFailure;
    }
    return result ? nTrue : nFalse;
  } catch (...) {
    getLogger()->error(ERR_EXC_RAISED);
  }
  return nGenericFailure;
}

int driver_dirExists(const char *sUrl) {
  try {
    getLogger()->info("Checking if directory exists at URL {}...", sUrl);
    if (!IsConnected()) {
      getLogger()->error("Cannot check if directory exists when disconnected.");
      return nGenericFailure;
    }
    if (!sUrl) {
      getLogger()->error(ERR_NULL_ARG, __func__, STRINGIFY(sUrl));
      return nGenericFailure;
    }
    bool result;
    if (driver->Exists(&result, sUrl)) {
      return nGenericFailure;
    }
    return result ? nTrue : nFalse;
  } catch (...) {
    getLogger()->error(ERR_EXC_RAISED);
  }
  return nGenericFailure;
}

long long int driver_getFileSize(const char *sUrl) {
  try {
    getLogger()->info("Retrieving size of file at URL {}...", sUrl);
    if (!IsConnected()) {
      getLogger()->error("Cannot get object size when disconnected.");
      return nSizeFailure;
    }
    if (!sUrl) {
      getLogger()->error(ERR_NULL_ARG, __func__, STRINGIFY(sUrl));
      return nSizeFailure;
    }
    size_t result;
    if (driver->GetSize(&result, sUrl)) {
      return nSizeFailure;
    }
    return static_cast<long long int>(result);
  } catch (...) {
    getLogger()->error(ERR_EXC_RAISED);
  }
  return nSizeFailure;
}

void *driver_fopen(const char *sUrl, char mode) {
  try {
    getLogger()->info("Opening file at URL {} in mode {}...", sUrl, mode);
    if (!IsConnected()) {
      getLogger()->error("Cannot open file when disconnected.");
      return nullptr;
    }
    if (!sUrl) {
      getLogger()->error(ERR_NULL_ARG, __func__, STRINGIFY(sUrl));
      return nullptr;
    }
    FileStream *fsPtr;
    switch (mode) {
    case 'r':
      if (driver->OpenForReading(&fsPtr, sUrl)) {
        return nullptr;
      }
      break;
    case 'w':
      if (driver->OpenForWriting(&fsPtr, sUrl)) {
        return nullptr;
      }
      break;
    case 'a':
      if (driver->OpenForAppending(&fsPtr, sUrl)) {
        return nullptr;
      }
      break;
    default:
      getLogger()->error(ERR_INVALID_FSTREAM_MODE, sUrl, mode);
      return nullptr;
    }
    return fsPtr->GetHandle();
  } catch (...) {
    getLogger()->error(ERR_EXC_RAISED);
  }
  return nullptr;
}

int driver_fclose(void *handle) {
  try {
    getLogger()->info("Closing file with handle {}...", handle);
    if (!IsConnected()) {
      getLogger()->error("Cannot close file when disconnected.");
      return nCloseFailure;
    }
    if (!handle) {
      getLogger()->error(ERR_NULL_ARG, __func__, STRINGIFY(handle));
      return nCloseFailure;
    }
    if (driver->Close(handle)) {
      return nCloseFailure;
    }
    return nCloseSuccess;
  } catch (...) {
    getLogger()->error(ERR_EXC_RAISED);
  }
  return nCloseFailure;
}

long long int driver_fread(void *dest, size_t size, size_t count,
                           void *handle) {
  try {
    getLogger()->info("Reading {}x{} bytes from file with handle {} to {}...",
                      size, count, handle, dest);
    if (!IsConnected()) {
      getLogger()->error("Cannot read from file when disconnected.");
      return nReadFailure;
    }
    if (!dest) {
      getLogger()->error(ERR_NULL_ARG, __func__, STRINGIFY(dest));
      return nReadFailure;
    }
    if (!handle) {
      getLogger()->error(ERR_NULL_ARG, __func__, STRINGIFY(handle));
      return nReadFailure;
    }
    size_t nRead;
    if (driver->Read(&nRead, handle, dest, size, count)) {
      return nReadFailure;
    }
    if (nRead == 0ULL) {
      return nReadFailure;
    }
    return nRead;
  } catch (...) {
    getLogger()->error(ERR_EXC_RAISED);
  }
  return nReadFailure;
}

int driver_fseek(void *handle, long long int offset, int whence) {
  try {
    getLogger()->info(
        "Seeking offset {} from origin {} in file with handle {}...", offset,
        whence, handle);
    if (!IsConnected()) {
      getLogger()->error("Cannot seek into file when disconnected.");
      return nSeekFailure;
    }
    if (!handle) {
      getLogger()->error(ERR_NULL_ARG, __func__, STRINGIFY(handle));
      return nSeekFailure;
    }
    int nOrigin;
    switch (whence) {
    case 0:
      nOrigin = ios::beg;
      break;
    case 1:
      nOrigin = ios::cur;
      break;
    case 2:
      nOrigin = ios::end;
      break;
    default:
      getLogger()->error(ERR_INVALID_SEEK_ORIGIN, whence);
      return nSeekFailure;
    }
    if (driver->Seek(handle, offset, nOrigin)) {
      return nSeekFailure;
    }
    return nSeekSuccess;
  } catch (...) {
    getLogger()->error(ERR_EXC_RAISED);
  }
  return nSeekFailure;
}

const char *driver_getlasterror() {
  try {
    getLogger()->info("Retrieving last error...");
    const string &logstring = ::az::logging::getLastError();
    if (logstring.empty()) {
      return nullptr;
    }
    return logstring.c_str();
  } catch (...) {
    getLogger()->error(ERR_EXC_RAISED);
  }
  return "Error while trying to fetch last error.";
}

long long int driver_fwrite(const void *source, size_t size, size_t count,
                            void *handle) {
  try {
    getLogger()->info("Writing {}x{} bytes from {} to file with handle {}...",
                      size, count, source, handle);
    if (!IsConnected()) {
      getLogger()->error("Cannot write to file when disconnected.");
      return nWriteFailure;
    }
    if (!source) {
      getLogger()->error(ERR_NULL_ARG, __func__, STRINGIFY(source));
      return nWriteFailure;
    }
    if (!handle) {
      getLogger()->error(ERR_NULL_ARG, __func__, STRINGIFY(handle));
      return nWriteFailure;
    }
    size_t nWritten;
    if (driver->Write(&nWritten, handle, source, size, count)) {
      return nWriteFailure;
    }
    return nWritten;
  } catch (...) {
    getLogger()->error(ERR_EXC_RAISED);
  }
  return nWriteFailure;
}

int driver_fflush(void *handle) {
  try {
    getLogger()->info("Flushing file with handle {}...", handle);
    if (!IsConnected()) {
      getLogger()->error("Cannot flush file when disconnected.");
      return nFlushFailure;
    }
    if (!handle) {
      getLogger()->error(ERR_NULL_ARG, __func__, STRINGIFY(handle));
      return nFlushFailure;
    }
    if (driver->Flush(handle)) {
      return nFlushFailure;
    }
    return nFlushSuccess;
  } catch (...) {
    getLogger()->error(ERR_EXC_RAISED);
  }
  return nFlushFailure;
}

int driver_remove(const char *sUrl) {
  try {
    getLogger()->info("Removing file at URL {}...", sUrl);
    if (!IsConnected()) {
      getLogger()->error("Cannot remove file when disconnected.");
      return nFailure;
    }
    if (!sUrl) {
      getLogger()->error(ERR_NULL_ARG, __func__, STRINGIFY(sUrl));
      return nFailure;
    }
    if (driver->Remove(sUrl)) {
      return nFailure;
    }
    return nSuccess;
  } catch (...) {
    getLogger()->error(ERR_EXC_RAISED);
  }
  return nFailure;
}

int driver_mkdir(const char *sUrl) {
  try {
    getLogger()->info("Creating directory at URL {}...", sUrl);
    if (!IsConnected()) {
      getLogger()->error("Cannot make a directory when disconnected.");
      return nFailure;
    }
    if (!sUrl) {
      getLogger()->error(ERR_NULL_ARG, __func__, STRINGIFY(sUrl));
      return nFailure;
    }
    if (driver->MkDir(sUrl)) {
      return nFailure;
    }
    return nSuccess;
  } catch (...) {
    getLogger()->error(ERR_EXC_RAISED);
  }
  return nFailure;
}

int driver_rmdir(const char *sUrl) {
  try {
    getLogger()->info("Removing directory at URL {}...", sUrl);
    if (!IsConnected()) {
      getLogger()->error("Cannot remove directory when disconnected.");
      return nFailure;
    }
    if (!sUrl) {
      getLogger()->error(ERR_NULL_ARG, __func__, STRINGIFY(sUrl));
      return nFailure;
    }
    if (driver->RmDir(sUrl)) {
      return nFailure;
    }
    return nSuccess;
  } catch (...) {
    getLogger()->error(ERR_EXC_RAISED);
  }
  return nFailure;
}

long long int driver_diskFreeSpace(const char *sUrl) {
  try {
    getLogger()->info("Retrieving free disk space at URL {}...", sUrl);
    if (!IsConnected()) {
      getLogger()->error("Cannot get free disk space when disconnected.");
      return nFreeDiskSpaceFailure;
    }
    if (!sUrl) {
      getLogger()->error(ERR_NULL_ARG, __func__, STRINGIFY(sUrl));
      return nFreeDiskSpaceFailure;
    }
    size_t nResult;
    if (driver->GetFreeDiskSpace(&nResult)) {
      return nFreeDiskSpaceFailure;
    }
    return nResult;
  } catch (...) {
    getLogger()->error(ERR_EXC_RAISED);
  }
  return nFreeDiskSpaceFailure;
}

int driver_copyToLocal(const char *sSourceUrl, const char *sDestUrl) {
  try {
    getLogger()->info("Copying file at URL {} to URL {}...", sSourceUrl,
                      sDestUrl);
    if (!IsConnected()) {
      getLogger()->error("Cannot copy to a local file when disconnected.");
      return nFailure;
    }
    if (!sSourceUrl) {
      getLogger()->error(ERR_NULL_ARG, __func__, STRINGIFY(sSourceUrl));
      return nFailure;
    }
    if (!sDestUrl) {
      getLogger()->error(ERR_NULL_ARG, __func__, STRINGIFY(sDestUrl));
      return nFailure;
    }
    if (driver->CopyTo(sSourceUrl, sDestUrl)) {
      return nFailure;
    }
    return nSuccess;
  } catch (...) {
    getLogger()->error(ERR_EXC_RAISED);
  }
  return nFailure;
}

int driver_copyFromLocal(const char *sSourceUrl, const char *sDestUrl) {
  try {
    getLogger()->info("Copying file at URL {} to URL {}...", sSourceUrl,
                      sDestUrl);
    if (!IsConnected()) {
      getLogger()->error("Cannot copy from a local file when disconnected.");
      return nFailure;
    }
    if (!sSourceUrl) {
      getLogger()->error(ERR_NULL_ARG, __func__, STRINGIFY(sSourceUrl));
      return nFailure;
    }
    if (!sDestUrl) {
      getLogger()->error(ERR_NULL_ARG, __func__, STRINGIFY(sDestUrl));
      return nFailure;
    }
    if (driver->CopyFrom(sDestUrl, sSourceUrl)) {
      return nFailure;
    }
    return nSuccess;
  } catch (...) {
    getLogger()->error(ERR_EXC_RAISED);
  }
  return nFailure;
}

int driver_concat(const char *destfilename, const char **sourcefilenames,
                  size_t sourcefilecount) {
  try {
    getLogger()->info("Concatenating {} files to URL {}...", sourcefilecount,
                      destfilename);
    if (!IsConnected()) {
      getLogger()->error("Cannot concatenate objects when disconnected.");
      return nFailure;
    }
    for (size_t i = 0; i < sourcefilecount; i++) {
      getLogger()->info("  Source file #{}: {}", i + 1, sourcefilenames[i]);
    }
    if (!destfilename) {
      getLogger()->error(ERR_NULL_ARG, __func__, STRINGIFY(destfilename));
      return nFailure;
    }
    if (!sourcefilenames) {
      getLogger()->error(ERR_NULL_ARG, __func__, STRINGIFY(sourcefilenames));
      return nFailure;
    }
    if (driver->Concatenate(
            vector<string>(sourcefilenames, sourcefilenames + sourcefilecount),
            destfilename)) {
      return nFailure;
    }
    return nSuccess;
  } catch (...) {
    getLogger()->error(ERR_EXC_RAISED);
  }
  return nFailure;
}
