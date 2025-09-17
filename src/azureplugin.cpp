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

static Driver driver;
static util::errlog::ErrorLogger errorLogger;

const char* driver_getDriverName()
{
	HANDLE_ERRORS(nullptr,
	{
		spdlog::debug("Retrieving driver name");
		return driver.GetName().c_str();
	})
}

const char* driver_getVersion()
{
	HANDLE_ERRORS(nullptr,
	{
		spdlog::debug("Retrieving driver version");
		return driver.GetVersion().c_str();
	})
}

const char* driver_getScheme()
{
	HANDLE_ERRORS(nullptr,
	{
		spdlog::debug("Retrieving driver scheme");
		return driver.GetScheme().c_str();
	})
}

int driver_isReadOnly()
{
	HANDLE_ERRORS(nGenericFailure,
	{
		spdlog::debug("Retrieving read-only state");
		return driver.IsReadOnly();
	})
}

int driver_connect()
{
	HANDLE_ERRORS(nFailure,
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
	})
}

int driver_disconnect()
{
	HANDLE_ERRORS(nFailure,
	{
		spdlog::debug("Disconnecting");
		driver.Disconnect();
		return nSuccess;
	})
}

int driver_isConnected()
{
	HANDLE_ERRORS(nGenericFailure,
	{
		spdlog::debug("Retrieving connection state");
		return driver.IsConnected();
	})
}

long long int driver_getSystemPreferredBufferSize()
{
	HANDLE_ERRORS(nGenericFailure,
	{
		spdlog::debug("Retrieving preferred buffer size");
		return driver.GetPreferredBufferSize();
	})
}

int driver_fileExists(const char* sUrl)
{
	HANDLE_ERRORS(nGenericFailure,
	{
		spdlog::debug("Checking if file exists at URL {}", sUrl);
		if (!sUrl) throw NullArgError(__func__, STRINGIFY(sUrl));
		return driver.Exists(sUrl) ? nTrue : nFalse;
	})
}

int driver_dirExists(const char* sUrl)
{
	HANDLE_ERRORS(nGenericFailure,
	{
		spdlog::debug("Checking if directory exists at URL {}", sUrl);
		if (!sUrl) throw NullArgError(__func__, STRINGIFY(sUrl));
		return driver.Exists(sUrl) ? nTrue : nFalse;
	})
}

long long int driver_getFileSize(const char* sUrl)
{
	HANDLE_ERRORS(nSizeFailure,
	{
		spdlog::debug("Retrieving size of file at URL {}", sUrl);
		if (!sUrl) throw NullArgError(__func__, STRINGIFY(sUrl));
		return driver.GetSize(sUrl);
	})
}

void* driver_fopen(const char* sUrl, char mode)
{
	HANDLE_ERRORS(nullptr,
	{
		spdlog::debug("Opening file at URL {} in mode {}", sUrl, mode);
		if (!sUrl) throw NullArgError(__func__, STRINGIFY(sUrl));
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
	})
}

int driver_fclose(void* handle)
{
	HANDLE_ERRORS(nCloseFailure,
	{
		spdlog::debug("Closing file with handle {}", handle);
		if (!handle) throw NullArgError(__func__, STRINGIFY(handle));
		driver.Close(handle);
		return nCloseSuccess;
	})
}

long long int driver_fread(void* dest, size_t size, size_t count, void* handle)
{
	HANDLE_ERRORS(nReadFailure,
	{
		spdlog::debug("Reading {}*{} bytes from file with handle {} to {}", size, count, handle, dest);
		if (!dest) throw NullArgError(__func__, STRINGIFY(dest));
		if (!handle) throw NullArgError(__func__, STRINGIFY(handle));
		return driver.Read(handle, dest, size, count);
	})
}

int driver_fseek(void* handle, long long int offset, int whence)
{
	HANDLE_ERRORS(nSeekFailure,
	{
		spdlog::debug("Seeking offset {} from origin {} in file with handle {}", offset, whence, handle);
		if (!handle) throw NullArgError(__func__, STRINGIFY(handle));
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
	})
}

const char* driver_getlasterror()
{
	HANDLE_ERRORS("error while trying to fetch last error",
	{
		spdlog::debug("Retrieving last error");
		const string& sLastError = errorLogger.GetLastError();
		if (sLastError.empty()) return nullptr;
		return sLastError.c_str();
	})
}
long long int driver_fwrite(const void* source, size_t size, size_t count, void* handle)
{
	HANDLE_ERRORS(nWriteFailure,
	{
		spdlog::debug("Writing {}*{} bytes from {} to file with handle {}", size, count, source, handle);
		if (!source) throw NullArgError(__func__, STRINGIFY(source));
		if (!handle) throw NullArgError(__func__, STRINGIFY(handle));
		return driver.Write(handle, source, size, count);
	})
}

int driver_fflush(void* handle)
{
	HANDLE_ERRORS(nFlushFailure,
	{
		spdlog::debug("Flushing file with handle {}", handle);
		if (!handle) throw NullArgError(__func__, STRINGIFY(handle));
		driver.Flush(handle);
		return nFlushSuccess;
	})
}

int driver_remove(const char* sUrl)
{
	HANDLE_ERRORS(nFailure,
	{
		spdlog::debug("Removing file at URL {}", sUrl);
		if (!sUrl) throw NullArgError(__func__, STRINGIFY(sUrl));
		driver.Remove(sUrl);
		return nSuccess;
	})
}

int driver_mkdir(const char* sUrl)
{
	HANDLE_ERRORS(nFailure,
	{
		spdlog::debug("Creating directory at URL {}", sUrl);
		if (!sUrl) throw NullArgError(__func__, STRINGIFY(sUrl));
		driver.MkDir(sUrl);
		return nSuccess;
	})
}

int driver_rmdir(const char* sUrl)
{
	HANDLE_ERRORS(nFailure,
	{
		spdlog::debug("Removing directory at URL {}", sUrl);
		if (!sUrl) throw NullArgError(__func__, STRINGIFY(sUrl));
		driver.RmDir(sUrl);
		return nSuccess;
	})
}

long long int driver_diskFreeSpace(const char* sUrl)
{
	HANDLE_ERRORS(nFreeDiskSpaceFailure,
	{
		spdlog::debug("Retrieving free disk space at URL {}", sUrl);
		if (!sUrl) throw NullArgError(__func__, STRINGIFY(sUrl));
		return driver.GetFreeDiskSpace();
	})
}

int driver_copyToLocal(const char* sSourceUrl, const char* sDestUrl)
{
	HANDLE_ERRORS(nFailure,
	{
		spdlog::debug("Copying file at URL {} to URL {}", sSourceUrl, sDestUrl);
		if (!sSourceUrl) throw NullArgError(__func__, STRINGIFY(sSourceUrl));
		if (!sDestUrl) throw NullArgError(__func__, STRINGIFY(sDestUrl));
		driver.CopyTo(sSourceUrl, sDestUrl);
		return nSuccess;
	})
}

int driver_copyFromLocal(const char* sSourceUrl, const char* sDestUrl)
{
	HANDLE_ERRORS(nFailure,
	{
		spdlog::debug("Copying file at URL {} to URL {}", sSourceUrl, sDestUrl);
		if (!sSourceUrl) throw NullArgError(__func__, STRINGIFY(sSourceUrl));
		if (!sDestUrl) throw NullArgError(__func__, STRINGIFY(sDestUrl));
		driver.CopyFrom(sDestUrl, sSourceUrl);
		return nSuccess;
	})
}
