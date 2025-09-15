#ifdef __CYGWIN__
#define _CRT_SECURE_NO_WARNINGS
#endif

#include "azureplugin.hpp"

#include <algorithm>
#include <assert.h>
#include <ios>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <iterator>
#include <limits>
#include <limits.h>
#include <memory>
#include <sstream>

#include "spdlog/spdlog.h"

#include "returnval.hpp"
#include "util.hpp"
#include "driver.hpp"
#include "exception.hpp"

using namespace std;
using namespace az;

const char sCaughtNonExceptionValue[] = "unknown error (caught a value that is not a descendant of std::exception)";

Driver driver;
util::errlog::ErrorLogger errorLogger;

static int FileOrDirExists(const char* sUrl);

const char* driver_getDriverName()
{
	try
	{
		spdlog::debug("Retrieving driver name");
		return driver.GetName().c_str();
	}
	catch (const exception& exc)
	{
		errorLogger.LogException(exc);
		return nullptr;
	}
	catch (...)
	{
		errorLogger.LogError(sCaughtNonExceptionValue);
		return nullptr;
	}
}

const char* driver_getVersion()
{
	try
	{
		spdlog::debug("Retrieving driver version");
		return driver.GetVersion().c_str();
	}
	catch (const exception& exc)
	{
		errorLogger.LogException(exc);
		return nullptr;
	}
	catch (...)
	{
		errorLogger.LogError(sCaughtNonExceptionValue);
		return nullptr;
	}
}

const char* driver_getScheme()
{
	try
	{
		spdlog::debug("Retrieving driver scheme");
		return driver.GetScheme().c_str();
	}
	catch (const exception& exc)
	{
		errorLogger.LogException(exc);
		return nullptr;
	}
	catch (...)
	{
		errorLogger.LogError(sCaughtNonExceptionValue);
		return nullptr;
	}
}

int driver_isReadOnly()
{
	try
	{
		spdlog::debug("Retrieving read-only state");
		return driver.IsReadOnly();
	}
	catch (const exception& exc)
	{
		errorLogger.LogException(exc);
		return nGenericFailure;
	}
	catch (...)
	{
		errorLogger.LogError(sCaughtNonExceptionValue);
		return nGenericFailure;
	}
}

int driver_connect()
{
	try
	{
		const string loglevel = util::env::GetEnvironmentVariableOrDefault("AZURE_DRIVER_LOGLEVEL", "info");
		if (loglevel == "debug")
		{
			spdlog::set_level(spdlog::level::debug);
		}
		else if (loglevel == "trace")
		{
			spdlog::set_level(spdlog::level::trace);
		}
		else
		{
			spdlog::set_level(spdlog::level::info);
		}

		spdlog::debug("Connecting");
		driver.Connect();
		return nSuccess;
	}
	catch (const exception& exc)
	{
		errorLogger.LogException(exc);
		return nFailure;
	}
	catch (...)
	{
		errorLogger.LogError(sCaughtNonExceptionValue);
		return nFailure;
	}
}

int driver_disconnect()
{
	try
	{
		spdlog::debug("Disconnecting");
		driver.Disconnect();
		return nSuccess;
	}
	catch (const exception& exc)
	{
		errorLogger.LogException(exc);
		return nFailure;
	}
	catch (...)
	{
		errorLogger.LogError(sCaughtNonExceptionValue);
		return nFailure;
	}
}

int driver_isConnected()
{
	try
	{
		spdlog::debug("Retrieving connection state");
		return driver.IsConnected();
	}
	catch (const exception& exc)
	{
		errorLogger.LogException(exc);
		return nGenericFailure;
	}
	catch (...)
	{
		errorLogger.LogError(sCaughtNonExceptionValue);
		return nGenericFailure;
	}
}

long long int driver_getSystemPreferredBufferSize()
{
	try
	{
		spdlog::debug("Retrieving preferred buffer size");
		return driver.GetPreferredBufferSize();
	}
	catch (const exception& exc)
	{
		errorLogger.LogException(exc);
		return nGenericFailure;
	}
	catch (...)
	{
		errorLogger.LogError(sCaughtNonExceptionValue);
		return nGenericFailure;
	}
}

int driver_fileExists(const char* sUrl)
{
	try
	{
		spdlog::debug("Checking if file exists at URL {}", sUrl);
		if (!sUrl)
		{
			throw NullArgError(__func__, STRINGIFY(sUrl));
		}
		return FileOrDirExists(sUrl);
	}
	catch (const exception& exc)
	{
		errorLogger.LogException(exc);
		return nGenericFailure;
	}
	catch (...)
	{
		errorLogger.LogError(sCaughtNonExceptionValue);
		return nGenericFailure;
	}
}

int driver_dirExists(const char* sUrl)
{
	try
	{
		spdlog::debug("Checking if directory exists at URL {}", sUrl);
		if (!sUrl)
		{
			throw NullArgError(__func__, STRINGIFY(sUrl));
		}
		return FileOrDirExists(sUrl);
	}
	catch (const exception& exc)
	{
		errorLogger.LogException(exc);
		return nGenericFailure;
	}
	catch (...)
	{
		errorLogger.LogError(sCaughtNonExceptionValue);
		return nGenericFailure;
	}
}

long long int driver_getFileSize(const char* sUrl)
{
	try
	{
		spdlog::debug("Retrieving size of file at URL {}", sUrl);
		if (!sUrl)
		{
			throw NullArgError(__func__, STRINGIFY(sUrl));
		}
		return driver.GetSize(sUrl);
	}
	catch (const exception& exc)
	{
		errorLogger.LogException(exc);
		return nSizeFailure;
	}
	catch (...)
	{
		errorLogger.LogError(sCaughtNonExceptionValue);
		return nSizeFailure;
	}
}

void* driver_fopen(const char* sUrl, char mode)
{
	try
	{
		spdlog::debug("Opening file at URL {} in mode {}", sUrl, mode);
		if (!sUrl)
		{
			throw NullArgError(__func__, STRINGIFY(sUrl));
		}
		switch (mode)
		{
		case 'r':
			return driver.OpenForReading(sUrl).GetHandle();
		case 'w':
			return driver.OpenForWriting(sUrl).GetHandle();
		case 'a':
			return driver.OpenForAppending(sUrl).GetHandle();
		default:
			throw InvalidFileStreamModeError(sUrl, mode);
		}
	}
	catch (const exception& exc)
	{
		errorLogger.LogException(exc);
		return nullptr;
	}
	catch (...)
	{
		errorLogger.LogError(sCaughtNonExceptionValue);
		return nullptr;
	}
}

int driver_fclose(void* handle)
{
	try
	{
		spdlog::debug("Closing file with handle {}", handle);
		if (!handle)
		{
			throw NullArgError(__func__, STRINGIFY(handle));
		}
		driver.Close(handle);
		return nCloseSuccess;
	}
	catch (const exception& exc)
	{
		errorLogger.LogException(exc);
		return nCloseFailure;
	}
	catch (...)
	{
		errorLogger.LogError(sCaughtNonExceptionValue);
		return nCloseFailure;
	}
}

long long int driver_fread(void* dest, size_t size, size_t count, void* handle)
{
	try
	{
		spdlog::debug("Reading {}*{} bytes from file with handle {} to {}", size, count, handle, dest);
		if (!dest)
		{
			throw NullArgError(__func__, STRINGIFY(dest));
		}
		if (!handle)
		{
			throw NullArgError(__func__, STRINGIFY(handle));
		}
		return driver.Read(handle, dest, size, count);
	}
	catch (const exception& exc)
	{
		errorLogger.LogException(exc);
		return nReadFailure;
	}
	catch (...)
	{
		errorLogger.LogError(sCaughtNonExceptionValue);
		return nReadFailure;
	}
}

int driver_fseek(void* handle, long long int offset, int whence)
{
	try
	{
		spdlog::debug("Seeking offset {} from {} in file with handle {}", offset, whence, handle);
		if (!handle)
		{
			throw NullArgError(__func__, STRINGIFY(handle));
		}
		int nOrigin;
		switch (whence)
		{
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
			throw InvalidSeekOriginError(whence);
		}
		driver.Seek(handle, offset, nOrigin);
		return nSeekSuccess;
	}
	catch (const exception& exc)
	{
		errorLogger.LogException(exc);
		return nSeekFailure;
	}
	catch (...)
	{
		errorLogger.LogError(sCaughtNonExceptionValue);
		return nSeekFailure;
	}
}

const char* driver_getlasterror()
{
	try
	{
		spdlog::debug("Retrieving last error");
		const string& sLastError = errorLogger.GetLastError();
		if (sLastError.empty())
		{
			return nullptr;
		}
		return sLastError.c_str();
	}
	catch (const exception& exc)
	{
		errorLogger.LogException(exc);
		return "error while trying to fetch last error";
	}
	catch (...)
	{
		errorLogger.LogError(sCaughtNonExceptionValue);
		return "error while trying to fetch last error";
	}
}
long long int driver_fwrite(const void* source, size_t size, size_t count, void* handle)
{
	try
	{
		spdlog::debug("Writing {}*{} bytes from {} to file with handle {}", size, count, source, handle);
		if (!source)
		{
			throw NullArgError(__func__, STRINGIFY(source));
		}
		if (!handle)
		{
			throw NullArgError(__func__, STRINGIFY(handle));
		}
		return driver.Write(handle, source, size, count);
	}
	catch (const exception& exc)
	{
		errorLogger.LogException(exc);
		return nWriteFailure;
	}
	catch (...)
	{
		errorLogger.LogError(sCaughtNonExceptionValue);
		return nWriteFailure;
	}
}

int driver_fflush(void* handle)
{
	try
	{
		spdlog::debug("Flushing file with handle {}", handle);
		if (!handle)
		{
			throw NullArgError(__func__, STRINGIFY(handle));
		}
		driver.Flush(handle);
		return nFlushSuccess;
	}
	catch (const exception& exc)
	{
		errorLogger.LogException(exc);
		return nFlushFailure;
	}
	catch (...)
	{
		errorLogger.LogError(sCaughtNonExceptionValue);
		return nFlushFailure;
	}
}

int driver_remove(const char* sUrl)
{
	try
	{
		spdlog::debug("Removing file at URL {}", sUrl);
		if (!sUrl)
		{
			throw NullArgError(__func__, STRINGIFY(sUrl));
		}
		driver.Remove(sUrl);
		return nSuccess;
	}
	catch (const exception& exc)
	{
		errorLogger.LogException(exc);
		return nFailure;
	}
	catch (...)
	{
		errorLogger.LogError(sCaughtNonExceptionValue);
		return nFailure;
	}
}

int driver_mkdir(const char* sUrl)
{
	try
	{
		spdlog::debug("Creating directory at URL {}", sUrl);
		if (!sUrl)
		{
			throw NullArgError(__func__, STRINGIFY(sUrl));
		}
		driver.MkDir(sUrl);
		return nSuccess;
	}
	catch (const exception& exc)
	{
		errorLogger.LogException(exc);
		return nFailure;
	}
	catch (...)
	{
		errorLogger.LogError(sCaughtNonExceptionValue);
		return nFailure;
	}
}

int driver_rmdir(const char* sUrl)
{
	try
	{
		spdlog::debug("Removing directory at URL {}", sUrl);
		if (!sUrl)
		{
			throw NullArgError(__func__, STRINGIFY(sUrl));
		}
		driver.RmDir(sUrl);
		return nSuccess;
	}
	catch (const exception& exc)
	{
		errorLogger.LogException(exc);
		return nFailure;
	}
	catch (...)
	{
		errorLogger.LogError(sCaughtNonExceptionValue);
		return nFailure;
	}
}

long long int driver_diskFreeSpace(const char* sUrl)
{
	try
	{
		spdlog::debug("Retrieving free disk space at URL {}", sUrl);
		if (!sUrl)
		{
			throw NullArgError(__func__, STRINGIFY(sUrl));
		}
		return driver.GetFreeDiskSpace();
	}
	catch (const exception& exc)
	{
		errorLogger.LogException(exc);
		return nFreeDiskSpaceFailure;
	}
	catch (...)
	{
		errorLogger.LogError(sCaughtNonExceptionValue);
		return nFreeDiskSpaceFailure;
	}
}

int driver_copyToLocal(const char* sSourceUrl, const char* sDestUrl)
{
	try
	{
		spdlog::debug("Copying file at URL {} to URL {}", sSourceUrl, sDestUrl);
		if (!sSourceUrl)
		{
			throw NullArgError(__func__, STRINGIFY(sSourceUrl));
		}
		if (!sDestUrl)
		{
			throw NullArgError(__func__, STRINGIFY(sDestUrl));
		}
		driver.CopyTo(sSourceUrl, sDestUrl);
		return nSuccess;
	}
	catch (const exception& exc)
	{
		errorLogger.LogException(exc);
		return nFailure;
	}
	catch (...)
	{
		errorLogger.LogError(sCaughtNonExceptionValue);
		return nFailure;
	}
}

int driver_copyFromLocal(const char* sSourceUrl, const char* sDestUrl)
{
	try
	{
		spdlog::debug("Copying file at URL {} to URL {}", sSourceUrl, sDestUrl);
		if (!sSourceUrl)
		{
			throw NullArgError(__func__, STRINGIFY(sSourceUrl));
		}
		if (!sDestUrl)
		{
			throw NullArgError(__func__, STRINGIFY(sDestUrl));
		}
		driver.CopyFrom(sDestUrl, sSourceUrl);
		return nSuccess;
	}
	catch (const exception& exc)
	{
		errorLogger.LogException(exc);
		return nFailure;
	}
	catch (...)
	{
		errorLogger.LogError(sCaughtNonExceptionValue);
		return nFailure;
	}
}

static int FileOrDirExists(const char* sUrl)
{
	return driver.Exists(sUrl) ? nTrue : nFalse;
}
