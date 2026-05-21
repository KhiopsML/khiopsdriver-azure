/*
This is the main file that implements the functions exposed by the driver as
a library. It delegates most of the work to the Driver class. This library
must be C-compatible so it provides a C interface. This means that all
high-level types taken as arguments or returned by the driver are converted,
in this file, to basic C types.
*/

#ifdef __CYGWIN__
#define _CRT_SECURE_NO_WARNINGS
#endif

#include "azureplugin.hpp"
#include "driver.hpp"
#include "khiops_driver_common/logging.hpp"
#include "khiops_driver_common/util.hpp"
#include "khiops_driver_common/logging.hpp"
#include "returnval.hpp"
#include <memory>
#include <spdlog/spdlog.h>

// Used to log function argument names.
#define STRINGIFY(s) #s

using namespace std;
using namespace khiops_driver_azure;
using namespace khiops_driver_common;
// Use this function to get the logger that should be used to log anything.
using khiops_driver_common::logging::getLogger;

/*********************
 * THE DRIVER OBJECT *
 *********************/

static unique_ptr<Driver> driver = nullptr;
static bool IsConnected() { return driver != nullptr; }

/******************************************
 * PREFERRED BUFFER SIZE LAZY INITIALIZER *
 ******************************************/

namespace {
constexpr long long int DEFAULT_PREFERRED_BUFFER_SIZE = 4LL * 1024LL * 1024LL;
size_t nPreferredBufferSize;

class PreferredBufferSizeInitializer {
public:
  PreferredBufferSizeInitializer() {
    string envvarval = util::env::GetEnvVar("AZURE_PREFERRED_BUFFER_SIZE");
    if (envvarval.empty()) {
      nPreferredBufferSize = DEFAULT_PREFERRED_BUFFER_SIZE;
    } else {
      try {
        nPreferredBufferSize = stoull(envvarval);
      } catch (const invalid_argument &) {
        getLogger()->debug("Value {} of environment variable "
                           "AZURE_PREFERRED_BUFFER_SIZE is not "
                           "a valid number. Falling back to default {}...",
                           envvarval, DEFAULT_PREFERRED_BUFFER_SIZE);
      } catch (const out_of_range &) {
        getLogger()->debug("Value {} of environment variable "
                           "AZURE_PREFERRED_BUFFER_SIZE is out "
                           "of range. Falling back to default {}...",
                           envvarval, DEFAULT_PREFERRED_BUFFER_SIZE);
      }
    }
  }
};
} // namespace

static size_t GetSystemPreferredBufferSize() {
  static PreferredBufferSizeInitializer preferredBufferSizeInitializer;
  return nPreferredBufferSize;
}

/*****************
 * ERROR STRINGS *
 *****************/

static const char *ERR_NONEXC_RAISED =
    "An non-exception value has been raised as an exception.";
static const char *ERR_EXC_RAISED = "An exception has been raised: {}";
static const char *ERR_NULL_ARG =
    "Error calling '{}': passing null pointer as argument '{}'.";
static const char *ERR_INVALID_FSTREAM_MODE =
    "Tried to open file '{}' with invalid mode '{}'.";
static const char *ERR_INVALID_SEEK_ORIGIN =
    "Tried to seek from invalid origin '{}'.";

/***********************************************************************
 * DRIVER FUNCTIONS BEGIN HERE AND CONTINUE TILL THE END OF THIS FILE. *
 ***********************************************************************/

const char *driver_getDriverName() {
  try {
    getLogger()->info("Retrieving driver name...");
    return "Azure driver";
  } catch (const exception &exc) {
    getLogger()->error(ERR_EXC_RAISED, exc.what());
  } catch (...) {
    getLogger()->error(ERR_NONEXC_RAISED);
  }
  return nullptr;
}

const char *driver_getVersion() {
  try {
    getLogger()->info("Retrieving driver version...");
    return DRIVER_VERSION;
  } catch (const exception &exc) {
    getLogger()->error(ERR_EXC_RAISED, exc.what());
  } catch (...) {
    getLogger()->error(ERR_NONEXC_RAISED);
  }
  return nullptr;
}

const char *driver_getScheme() {
  try {
    getLogger()->info("Retrieving driver scheme...");
    return "https";
  } catch (const exception &exc) {
    getLogger()->error(ERR_EXC_RAISED, exc.what());
  } catch (...) {
    getLogger()->error(ERR_NONEXC_RAISED);
  }
  return nullptr;
}

int driver_isReadOnly() {
  try {
    getLogger()->info("Retrieving read-only state...");
    return kFalse;
  } catch (const exception &exc) {
    getLogger()->error(ERR_EXC_RAISED, exc.what());
  } catch (...) {
    getLogger()->error(ERR_NONEXC_RAISED);
  }
  return kFailure;
}

int driver_connect() {
  try {
    getLogger()->info("Connecting...");
    driver = make_unique<Driver>(GetSystemPreferredBufferSize());
    return kOtherSuccess;
  } catch (const exception &exc) {
    getLogger()->error(ERR_EXC_RAISED, exc.what());
  } catch (...) {
    getLogger()->error(ERR_NONEXC_RAISED);
  }
  return kOtherFailure;
}

int driver_disconnect() {
  try {
    getLogger()->info("Disconnecting...");
    if (!IsConnected()) {
      getLogger()->error("Cannot disconnect when already disconnected.");
      return kOtherFailure;
    }
    driver = nullptr;
    return kOtherSuccess;
  } catch (const exception &exc) {
    getLogger()->error(ERR_EXC_RAISED, exc.what());
  } catch (...) {
    getLogger()->error(ERR_NONEXC_RAISED);
  }
  return kOtherFailure;
}

int driver_isConnected() {
  try {
    getLogger()->info("Retrieving connection state...");
    return IsConnected() ? kTrue : kFalse;
  } catch (const exception &exc) {
    getLogger()->error(ERR_EXC_RAISED, exc.what());
  } catch (...) {
    getLogger()->error(ERR_NONEXC_RAISED);
  }
  return kFailure;
}

long long int driver_getSystemPreferredBufferSize() {
  try {
    getLogger()->info("Retrieving preferred buffer size...");
    return static_cast<long long int>(GetSystemPreferredBufferSize());
  } catch (const exception &exc) {
    getLogger()->error(ERR_EXC_RAISED, exc.what());
  } catch (...) {
    getLogger()->error(ERR_NONEXC_RAISED);
  }
  return kFailure;
}

int driver_fileExists(const char *sUrl) {
  try {
    getLogger()->info("Checking if file exists at URL {}...", sUrl);
    if (!IsConnected()) {
      getLogger()->error("Cannot check if file exists when disconnected.");
      return kFailure;
    }
    if (!sUrl) {
      getLogger()->error(ERR_NULL_ARG, __func__, STRINGIFY(sUrl));
      return kFailure;
    }
    bool result;
    if (driver->Exists(&result, sUrl)) {
      return kFailure;
    }
    return result ? kTrue : kFalse;
  } catch (const exception &exc) {
    getLogger()->error(ERR_EXC_RAISED, exc.what());
  } catch (...) {
    getLogger()->error(ERR_NONEXC_RAISED);
  }
  return kFailure;
}

int driver_dirExists(const char *sUrl) {
  try {
    getLogger()->info("Checking if directory exists at URL {}...", sUrl);
    if (!IsConnected()) {
      getLogger()->error("Cannot check if directory exists when disconnected.");
      return kFailure;
    }
    if (!sUrl) {
      getLogger()->error(ERR_NULL_ARG, __func__, STRINGIFY(sUrl));
      return kFailure;
    }
    bool result;
    if (driver->Exists(&result, sUrl)) {
      return kFailure;
    }
    return result ? kTrue : kFalse;
  } catch (const exception &exc) {
    getLogger()->error(ERR_EXC_RAISED, exc.what());
  } catch (...) {
    getLogger()->error(ERR_NONEXC_RAISED);
  }
  return kFailure;
}

long long int driver_getFileSize(const char *sUrl) {
  try {
    getLogger()->info("Retrieving size of file at URL {}...", sUrl);
    if (!IsConnected()) {
      getLogger()->error("Cannot get object size when disconnected.");
      return kFailure;
    }
    if (!sUrl) {
      getLogger()->error(ERR_NULL_ARG, __func__, STRINGIFY(sUrl));
      return kFailure;
    }
    size_t result;
    if (driver->GetSize(&result, sUrl)) {
      return kFailure;
    }
    return static_cast<long long int>(result);
  } catch (const exception &exc) {
    getLogger()->error(ERR_EXC_RAISED, exc.what());
  } catch (...) {
    getLogger()->error(ERR_NONEXC_RAISED);
  }
  return kFailure;
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
  } catch (const exception &exc) {
    getLogger()->error(ERR_EXC_RAISED, exc.what());
  } catch (...) {
    getLogger()->error(ERR_NONEXC_RAISED);
  }
  return nullptr;
}

int driver_fclose(void *handle) {
  try {
    getLogger()->info("Closing file with handle {}...", handle);
    if (!IsConnected()) {
      getLogger()->error("Cannot close file when disconnected.");
      return kFailure;
    }
    if (!handle) {
      getLogger()->error(ERR_NULL_ARG, __func__, STRINGIFY(handle));
      return kFailure;
    }
    if (driver->Close(handle)) {
      return kFailure;
    }
    return kSuccess;
  } catch (const exception &exc) {
    getLogger()->error(ERR_EXC_RAISED, exc.what());
  } catch (...) {
    getLogger()->error(ERR_NONEXC_RAISED);
  }
  return kFailure;
}

long long int driver_fread(void *dest, size_t size, size_t count,
                           void *handle) {
  try {
    getLogger()->info("Reading {}x{} bytes from file with handle {} to {}...",
                      size, count, handle, dest);
    if (!IsConnected()) {
      getLogger()->error("Cannot read from file when disconnected.");
      return kFailure;
    }
    if (!dest) {
      getLogger()->error(ERR_NULL_ARG, __func__, STRINGIFY(dest));
      return kFailure;
    }
    if (!handle) {
      getLogger()->error(ERR_NULL_ARG, __func__, STRINGIFY(handle));
      return kFailure;
    }
    size_t nRead;
    if (driver->Read(&nRead, handle, dest, size, count)) {
      return kFailure;
    }
    if (nRead == 0ULL) {
      return kFailure;
    }
    return static_cast<long long int>(nRead);
  } catch (const exception &exc) {
    getLogger()->error(ERR_EXC_RAISED, exc.what());
  } catch (...) {
    getLogger()->error(ERR_NONEXC_RAISED);
  }
  return kFailure;
}

int driver_fseek(void *handle, long long int offset, int whence) {
  try {
    getLogger()->info(
        "Seeking offset {} from origin {} in file with handle {}...", offset,
        whence, handle);
    if (!IsConnected()) {
      getLogger()->error("Cannot seek into file when disconnected.");
      return kFailure;
    }
    if (!handle) {
      getLogger()->error(ERR_NULL_ARG, __func__, STRINGIFY(handle));
      return kFailure;
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
      return kFailure;
    }
    if (driver->Seek(handle, offset, nOrigin)) {
      return kFailure;
    }
    return kSuccess;
  } catch (const exception &exc) {
    getLogger()->error(ERR_EXC_RAISED, exc.what());
  } catch (...) {
    getLogger()->error(ERR_NONEXC_RAISED);
  }
  return kFailure;
}

const char *driver_getlasterror() {
  try {
    getLogger()->info("Retrieving last error...");
    const string &logstring = khiops_driver_common::logging::getLastError();
    if (logstring.empty()) {
      return nullptr;
    }
    return logstring.c_str();
  } catch (const exception &exc) {
    getLogger()->error(ERR_EXC_RAISED, exc.what());
  } catch (...) {
    getLogger()->error(ERR_NONEXC_RAISED);
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
      return kFailure;
    }
    if (!source) {
      getLogger()->error(ERR_NULL_ARG, __func__, STRINGIFY(source));
      return kFailure;
    }
    if (!handle) {
      getLogger()->error(ERR_NULL_ARG, __func__, STRINGIFY(handle));
      return kFailure;
    }
    size_t nWritten;
    if (driver->Write(&nWritten, handle, source, size, count)) {
      return kFailure;
    }
    return static_cast<long long int>(nWritten);
  } catch (const exception &exc) {
    getLogger()->error(ERR_EXC_RAISED, exc.what());
  } catch (...) {
    getLogger()->error(ERR_NONEXC_RAISED);
  }
  return kFailure;
}

int driver_fflush(void *handle) {
  try {
    getLogger()->info("Flushing file with handle {}...", handle);
    if (!IsConnected()) {
      getLogger()->error("Cannot flush file when disconnected.");
      return kFailure;
    }
    if (!handle) {
      getLogger()->error(ERR_NULL_ARG, __func__, STRINGIFY(handle));
      return kFailure;
    }
    if (driver->Flush(handle)) {
      return kFailure;
    }
    return kSuccess;
  } catch (const exception &exc) {
    getLogger()->error(ERR_EXC_RAISED, exc.what());
  } catch (...) {
    getLogger()->error(ERR_NONEXC_RAISED);
  }
  return kFailure;
}

int driver_remove(const char *sUrl) {
  try {
    getLogger()->info("Removing file at URL {}...", sUrl);
    if (!IsConnected()) {
      getLogger()->error("Cannot remove file when disconnected.");
      return kOtherFailure;
    }
    if (!sUrl) {
      getLogger()->error(ERR_NULL_ARG, __func__, STRINGIFY(sUrl));
      return kOtherFailure;
    }
    if (driver->Remove(sUrl)) {
      return kOtherFailure;
    }
    return kOtherSuccess;
  } catch (const exception &exc) {
    getLogger()->error(ERR_EXC_RAISED, exc.what());
  } catch (...) {
    getLogger()->error(ERR_NONEXC_RAISED);
  }
  return kOtherFailure;
}

int driver_mkdir(const char *sUrl) {
  try {
    getLogger()->info("Creating directory at URL {}...", sUrl);
    if (!IsConnected()) {
      getLogger()->error("Cannot make a directory when disconnected.");
      return kOtherFailure;
    }
    if (!sUrl) {
      getLogger()->error(ERR_NULL_ARG, __func__, STRINGIFY(sUrl));
      return kOtherFailure;
    }
    if (driver->MkDir(sUrl)) {
      return kOtherFailure;
    }
    return kOtherSuccess;
  } catch (const exception &exc) {
    getLogger()->error(ERR_EXC_RAISED, exc.what());
  } catch (...) {
    getLogger()->error(ERR_NONEXC_RAISED);
  }
  return kOtherFailure;
}

int driver_rmdir(const char *sUrl) {
  try {
    getLogger()->info("Removing directory at URL {}...", sUrl);
    if (!IsConnected()) {
      getLogger()->error("Cannot remove directory when disconnected.");
      return kOtherFailure;
    }
    if (!sUrl) {
      getLogger()->error(ERR_NULL_ARG, __func__, STRINGIFY(sUrl));
      return kOtherFailure;
    }
    if (driver->RmDir(sUrl)) {
      return kOtherFailure;
    }
    return kOtherSuccess;
  } catch (const exception &exc) {
    getLogger()->error(ERR_EXC_RAISED, exc.what());
  } catch (...) {
    getLogger()->error(ERR_NONEXC_RAISED);
  }
  return kOtherFailure;
}

long long int driver_diskFreeSpace(const char *sUrl) {
  try {
    getLogger()->info("Retrieving free disk space at URL {}...", sUrl);
    if (!IsConnected()) {
      getLogger()->error("Cannot get free disk space when disconnected.");
      return kFailure;
    }
    if (!sUrl) {
      getLogger()->error(ERR_NULL_ARG, __func__, STRINGIFY(sUrl));
      return kFailure;
    }
    size_t nResult;
    if (driver->GetFreeDiskSpace(&nResult)) {
      return kFailure;
    }
    return nResult;
  } catch (const exception &exc) {
    getLogger()->error(ERR_EXC_RAISED, exc.what());
  } catch (...) {
    getLogger()->error(ERR_NONEXC_RAISED);
  }
  return kFailure;
}

int driver_copyToLocal(const char *sSourceUrl, const char *sDestUrl) {
  try {
    getLogger()->info("Copying file at URL {} to URL {}...", sSourceUrl,
                      sDestUrl);
    if (!IsConnected()) {
      getLogger()->error("Cannot copy to a local file when disconnected.");
      return kOtherFailure;
    }
    if (!sSourceUrl) {
      getLogger()->error(ERR_NULL_ARG, __func__, STRINGIFY(sSourceUrl));
      return kOtherFailure;
    }
    if (!sDestUrl) {
      getLogger()->error(ERR_NULL_ARG, __func__, STRINGIFY(sDestUrl));
      return kOtherFailure;
    }
    if (driver->CopyTo(sSourceUrl, sDestUrl)) {
      return kOtherFailure;
    }
    return kOtherSuccess;
  } catch (const exception &exc) {
    getLogger()->error(ERR_EXC_RAISED, exc.what());
  } catch (...) {
    getLogger()->error(ERR_NONEXC_RAISED);
  }
  return kOtherFailure;
}

int driver_copyFromLocal(const char *sSourceUrl, const char *sDestUrl) {
  try {
    getLogger()->info("Copying file at URL {} to URL {}...", sSourceUrl,
                      sDestUrl);
    if (!IsConnected()) {
      getLogger()->error("Cannot copy from a local file when disconnected.");
      return kOtherFailure;
    }
    if (!sSourceUrl) {
      getLogger()->error(ERR_NULL_ARG, __func__, STRINGIFY(sSourceUrl));
      return kOtherFailure;
    }
    if (!sDestUrl) {
      getLogger()->error(ERR_NULL_ARG, __func__, STRINGIFY(sDestUrl));
      return kOtherFailure;
    }
    if (driver->CopyFrom(sDestUrl, sSourceUrl)) {
      return kOtherFailure;
    }
    return kOtherSuccess;
  } catch (const exception &exc) {
    getLogger()->error(ERR_EXC_RAISED, exc.what());
  } catch (...) {
    getLogger()->error(ERR_NONEXC_RAISED);
  }
  return kOtherFailure;
}

int driver_concat(const char *destfilename, const char **sourcefilenames,
                  size_t sourcefilecount) {
  try {
    getLogger()->info("Concatenating {} files to URL {}...", sourcefilecount,
                      destfilename);
    for (size_t i = 0; i < sourcefilecount; i++) {
      getLogger()->info("  Source file #{}: {}", i + 1, sourcefilenames[i]);
    }
    if (!IsConnected()) {
      getLogger()->error("Cannot concatenate objects when disconnected.");
      return kOtherFailure;
    }
    if (!destfilename) {
      getLogger()->error(ERR_NULL_ARG, __func__, STRINGIFY(destfilename));
      return kOtherFailure;
    }
    if (!sourcefilenames) {
      getLogger()->error(ERR_NULL_ARG, __func__, STRINGIFY(sourcefilenames));
      return kOtherFailure;
    }
    if (driver->Concatenate(
            vector<string>(sourcefilenames, sourcefilenames + sourcefilecount),
            destfilename)) {
      return kOtherFailure;
    }
    return kOtherSuccess;
  } catch (const exception &exc) {
    getLogger()->error(ERR_EXC_RAISED, exc.what());
  } catch (...) {
    getLogger()->error(ERR_NONEXC_RAISED);
  }
  return kOtherFailure;
}
