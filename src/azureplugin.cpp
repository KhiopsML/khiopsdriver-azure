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
#include "returnval.hpp"
#include "util.hpp"
#include <memory>
#include <spdlog/spdlog.h>

#define STRINGIFY(s) #s  // Use to log function argument names.

using namespace std;
using namespace az;

static Driver driver;

static const char *ERR_EXC_RAISED = "An exception has been raised.";
static const char *ERR_NULL_ARG = "Error calling '{}': passing null pointer as argument '{}'.";
static const char *ERR_INVALID_FSTREAM_MODE = "Tried to open file '{}' with invalid mode '{}'.";
static const char *ERR_INVALID_SEEK_ORIGIN = "Tried to seek from invalid origin '{}'.";

const char *driver_getDriverName() {
  try {
    spdlog::info("Retrieving driver name...");
    return driver.GetName().c_str();
  } catch (...) {
    spdlog::error(ERR_EXC_RAISED);
  }
  return nullptr;
}

const char *driver_getVersion() {
  try {
    spdlog::info("Retrieving driver version...");
    return driver.GetVersion().c_str();
  } catch (...) {
    spdlog::error(ERR_EXC_RAISED);
  }
  return nullptr;
}

const char *driver_getScheme() {
  try {
    spdlog::info("Retrieving driver scheme...");
    return driver.GetScheme().c_str();
  } catch (...) {
    spdlog::error(ERR_EXC_RAISED);
  }
  return nullptr;
}

int driver_isReadOnly() {
  try {
    spdlog::info("Retrieving read-only state...");
    return driver.IsReadOnly();
  } catch (...) {
    spdlog::error(ERR_EXC_RAISED);
  }
  return nGenericFailure;
}

int driver_connect() {
  try {
    spdlog::info("Connecting...");
    driver.Connect();
    return nSuccess;
  } catch (...) {
    spdlog::error(ERR_EXC_RAISED);
  }
  return nFailure;
}

int driver_disconnect() {
  try {
    spdlog::info("Disconnecting...");
    if (driver.Disconnect()) {
      return nFailure;
    }
    return nSuccess;
  } catch (...) {
    spdlog::error(ERR_EXC_RAISED);
  }
  return nFailure;
}

int driver_isConnected() {
  try {
    spdlog::info("Retrieving connection state...");
    return driver.IsConnected();
  } catch (...) {
    spdlog::error(ERR_EXC_RAISED);
  }
  return nGenericFailure;
}

long long int driver_getSystemPreferredBufferSize() {
  try {
    spdlog::info("Retrieving preferred buffer size...");
    return driver.GetPreferredBufferSize();
  } catch (...) {
    spdlog::error(ERR_EXC_RAISED);
  }
  return nGenericFailure;
}

int driver_fileExists(const char *sUrl) {
  try {
    spdlog::info("Checking if file exists at URL {}...", sUrl);
    if (!sUrl) {
      spdlog::error(ERR_NULL_ARG, __func__, STRINGIFY(sUrl));
      return nGenericFailure;
    }
    bool result;
    if (driver.Exists(&result, sUrl)) {
      return nGenericFailure;
    }
    return result ? nTrue : nFalse;
  } catch (...) {
    spdlog::error(ERR_EXC_RAISED);
  }
  return nGenericFailure;
}

int driver_dirExists(const char *sUrl) {
  try {
    spdlog::info("Checking if directory exists at URL {}...", sUrl);
    if (!sUrl) {
      spdlog::error(ERR_NULL_ARG, __func__, STRINGIFY(sUrl));
      return nGenericFailure;
    }
    bool result;
    if (driver.Exists(&result, sUrl)) {
      return nGenericFailure;
    }
    return result ? nTrue : nFalse;
  } catch (...) {
    spdlog::error(ERR_EXC_RAISED);
  }
  return nGenericFailure;
}

long long int driver_getFileSize(const char *sUrl) {
  try {
    spdlog::info("Retrieving size of file at URL {}...", sUrl);
    if (!sUrl) {
      spdlog::error(ERR_NULL_ARG, __func__, STRINGIFY(sUrl));
      return nSizeFailure;
    }
    size_t result;
    if (driver.GetSize(&result, sUrl)) {
      return nSizeFailure;
    }
    return static_cast<long long int>(result);
  } catch (...) {
    spdlog::error(ERR_EXC_RAISED);
  }
  return nSizeFailure;
}

void *driver_fopen(const char *sUrl, char mode) {
  try {
    spdlog::info("Opening file at URL {} in mode {}...", sUrl, mode);
    if (!sUrl) {
      spdlog::error(ERR_NULL_ARG, __func__, STRINGIFY(sUrl));
      return nullptr;
    }
    FileStream *fsPtr;
    switch (mode) {
    case 'r':
      if (driver.OpenForReading(&fsPtr, sUrl)) {
        return nullptr;
      }
      break;
    case 'w':
      if (driver.OpenForWriting(&fsPtr, sUrl)) {
        return nullptr;
      }
      break;
    case 'a':
      if (driver.OpenForAppending(&fsPtr, sUrl)) {
        return nullptr;
      }
      break;
    default:
      spdlog::error(ERR_INVALID_FSTREAM_MODE, sUrl, mode);
      return nullptr;
    }
    return fsPtr->GetHandle();
  } catch (...) {
    spdlog::error(ERR_EXC_RAISED);
  }
  return nullptr;
}

int driver_fclose(void *handle) {
  try {
    spdlog::info("Closing file with handle {}...", handle);
    if (!handle) {
      spdlog::error(ERR_NULL_ARG, __func__, STRINGIFY(handle));
      return nCloseFailure;
    }
    if (driver.Close(handle)) {
      return nCloseFailure;
    }
    return nCloseSuccess;
  } catch (...) {
    spdlog::error(ERR_EXC_RAISED);
  }
  return nCloseFailure;
}

long long int driver_fread(void *dest, size_t size, size_t count,
                           void *handle) {
  try {
    spdlog::info("Reading {}x{} bytes from file with handle {} to {}...", size,
                  count, handle, dest);
    if (!dest) {
      spdlog::error(ERR_NULL_ARG, __func__, STRINGIFY(dest));
      return nReadFailure;
    }
    if (!handle) {
      spdlog::error(ERR_NULL_ARG, __func__, STRINGIFY(handle));
      return nReadFailure;
    }
    size_t nRead;
    if (driver.Read(&nRead, handle, dest, size, count)) {
      return nReadFailure;
    }
    if (nRead == 0ULL) {
      return nReadFailure;
    }
    return nRead;
  } catch (...) {
    spdlog::error(ERR_EXC_RAISED);
  }
  return nReadFailure;
}

int driver_fseek(void *handle, long long int offset, int whence) {
  try {
    spdlog::info("Seeking offset {} from origin {} in file with handle {}...",
                  offset, whence, handle);
    if (!handle) {
      spdlog::error(ERR_NULL_ARG, __func__, STRINGIFY(handle));
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
      spdlog::error(ERR_INVALID_SEEK_ORIGIN, whence);
      return nSeekFailure;
    }
    if (driver.Seek(handle, offset, nOrigin)) {
      return nSeekFailure;
    }
    return nSeekSuccess;
  } catch (...) {
    spdlog::error(ERR_EXC_RAISED);
  }
  return nSeekFailure;
}

const char *driver_getlasterror() {
  try {
    spdlog::info("Retrieving last error...");
    string *lastErrorPtr;
    driver.GetLastError(&lastErrorPtr);
    if (lastErrorPtr->empty()) {
      return nullptr;
    }
    return lastErrorPtr->c_str();
  } catch (...) {
    spdlog::error(ERR_EXC_RAISED);
  }
  return "Error while trying to fetch last error.";
}

long long int driver_fwrite(const void *source, size_t size, size_t count,
                            void *handle) {
  try {
    spdlog::info("Writing {}x{} bytes from {} to file with handle {}...", size,
                  count, source, handle);
    if (!source) {
      spdlog::error(ERR_NULL_ARG, __func__, STRINGIFY(source));
      return nWriteFailure;
    }
    if (!handle) {
      spdlog::error(ERR_NULL_ARG, __func__, STRINGIFY(handle));
      return nWriteFailure;
    }
    size_t nWritten;
    if (driver.Write(&nWritten, handle, source, size, count)) {
      return nWriteFailure;
    }
    return nWritten;
  } catch (...) {
    spdlog::error(ERR_EXC_RAISED);
  }
  return nWriteFailure;
}

int driver_fflush(void *handle) {
  try {
    spdlog::info("Flushing file with handle {}...", handle);
    if (!handle) {
      spdlog::error(ERR_NULL_ARG, __func__, STRINGIFY(handle));
      return nFlushFailure;
    }
    if (driver.Flush(handle)) {
      return nFlushFailure;
    }
    return nFlushSuccess;
  } catch (...) {
    spdlog::error(ERR_EXC_RAISED);
  }
  return nFlushFailure;
}

int driver_remove(const char *sUrl) {
  try {
    spdlog::info("Removing file at URL {}...", sUrl);
    if (!sUrl) {
      spdlog::error(ERR_NULL_ARG, __func__, STRINGIFY(sUrl));
      return nFailure;
    }
    if (driver.Remove(sUrl)) {
      return nFailure;
    }
    return nSuccess;
  } catch (...) {
    spdlog::error(ERR_EXC_RAISED);
  }
  return nFailure;
}

int driver_mkdir(const char *sUrl) {
  try {
    spdlog::info("Creating directory at URL {}...", sUrl);
    if (!sUrl) {
      spdlog::error(ERR_NULL_ARG, __func__, STRINGIFY(sUrl));
      return nFailure;
    }
    if (driver.MkDir(sUrl)) {
      return nFailure;
    }
    return nSuccess;
  } catch (...) {
    spdlog::error(ERR_EXC_RAISED);
  }
  return nFailure;
}

int driver_rmdir(const char *sUrl) {
  try {
    spdlog::info("Removing directory at URL {}...", sUrl);
    if (!sUrl) {
      spdlog::error(ERR_NULL_ARG, __func__, STRINGIFY(sUrl));
      return nFailure;
    }
    if (driver.RmDir(sUrl)) {
      return nFailure;
    }
    return nSuccess;
  } catch (...) {
    spdlog::error(ERR_EXC_RAISED);
  }
  return nFailure;
}

long long int driver_diskFreeSpace(const char *sUrl) {
  try {
    spdlog::info("Retrieving free disk space at URL {}...", sUrl);
    if (!sUrl) {
      spdlog::error(ERR_NULL_ARG, __func__, STRINGIFY(sUrl));
      return nFreeDiskSpaceFailure;
    }
    size_t nResult;
    if (driver.GetFreeDiskSpace(&nResult)) {
      return nFreeDiskSpaceFailure;
    }
    return nResult;
  } catch (...) {
    spdlog::error(ERR_EXC_RAISED);
  }
  return nFreeDiskSpaceFailure;
}

int driver_copyToLocal(const char *sSourceUrl, const char *sDestUrl) {
  try {
    spdlog::info("Copying file at URL {} to URL {}...", sSourceUrl, sDestUrl);
    if (!sSourceUrl) {
      spdlog::error(ERR_NULL_ARG, __func__, STRINGIFY(sSourceUrl));
      return nFailure;
    }
    if (!sDestUrl) {
      spdlog::error(ERR_NULL_ARG, __func__, STRINGIFY(sDestUrl));
      return nFailure;
    }
    if (driver.CopyTo(sSourceUrl, sDestUrl)) {
      return nFailure;
    }
    return nSuccess;
  } catch (...) {
    spdlog::error(ERR_EXC_RAISED);
  }
  return nFailure;
}

int driver_copyFromLocal(const char *sSourceUrl, const char *sDestUrl) {
  try {
    spdlog::info("Copying file at URL {} to URL {}...", sSourceUrl, sDestUrl);
    if (!sSourceUrl) {
      spdlog::error(ERR_NULL_ARG, __func__, STRINGIFY(sSourceUrl));
      return nFailure;
    }
    if (!sDestUrl) {
      spdlog::error(ERR_NULL_ARG, __func__, STRINGIFY(sDestUrl));
      return nFailure;
    }
    if (driver.CopyFrom(sDestUrl, sSourceUrl)) {
      return nFailure;
    }
    return nSuccess;
  } catch (...) {
    spdlog::error(ERR_EXC_RAISED);
  }
  return nFailure;
}

int driver_concat(const char *destfilename, const char **sourcefilenames,
                  size_t sourcefilecount) {
  try {
    spdlog::info("Concatenating {} files to URL {}...", sourcefilecount,
                  destfilename);
    for (size_t i = 0; i < sourcefilecount; i++) {
      spdlog::info("  Source file #{}: {}", i + 1, sourcefilenames[i]);
    }
    if (!destfilename) {
      spdlog::error(ERR_NULL_ARG, __func__, STRINGIFY(destfilename));
      return nFailure;
    }
    if (!sourcefilenames) {
      spdlog::error(ERR_NULL_ARG, __func__, STRINGIFY(sourcefilenames));
      return nFailure;
    }
    if (driver.Concatenate(
             vector<string>(sourcefilenames, sourcefilenames + sourcefilecount),
             destfilename)) {
      return nFailure;
    }
    return nSuccess;
  } catch (...) {
    spdlog::error(ERR_EXC_RAISED);
  }
  return nFailure;
}
