#ifdef __CYGWIN__
#define _CRT_SECURE_NO_WARNINGS
#endif

#include "azureplugin.hpp"
#include "azureplugin_internal.hpp"
#include "contrib/globmatch.hpp"

#include <algorithm>
#include <assert.h>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <iterator>
#include <limits>
#include <limits.h>
#include <memory>
#include <sstream>

#include "spdlog/spdlog.h"

// Include the necessary SDK headers
#include <azure/core.hpp>
#include <azure/storage/blobs.hpp>
// Include to support file shares
#include <azure/storage/files/shares.hpp>

#include <azure/identity/chained_token_credential.hpp>
#include <azure/identity/environment_credential.hpp>
#include <azure/identity/workload_identity_credential.hpp>
#include <azure/identity/azure_cli_credential.hpp>
#include <azure/identity/managed_identity_credential.hpp>

#include "util/macro.hpp"
#include "driver.hpp"
#include "returnval.hpp"
#include "errorlogger.hpp"
#include "logging.hpp"
#include "exception.hpp"

using namespace std;
using namespace az;

constexpr char emulated_storage_connection_string[] =
	"DefaultEndpointsProtocol=http;"
	"AccountName=devstoreaccount1;"
	"AccountKey=Eby8vdM02xNOcqFlqUwJPLlmEtlCDXJ1OUzFT50uSRZ6IFsuFq2UVErCz4I6tq/K1SZFPTOtr/KBHBeksoGMGw==;"
	"BlobEndpoint=http://127.0.0.1:10000/devstoreaccount1;";

const char sCaughtNonExceptionValue[] = "unknown error (caught a value that is not a descendant of std::exception)";

Driver driver;
ErrorLogger errorLogger;

// Add appropriate using namespace directives
using namespace Azure::Storage;
using namespace Azure::Storage::Blobs;
using namespace Azure::Storage::Files::Shares;
using namespace Azure::Identity;


static int FileOrDirExists(const char* sUrl);

//StreamVec<Reader> active_reader_handles;
//StreamVec<Writer> active_writer_handles;

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
		ConfigureLogLevel();
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
		return driver.CreateFileAccessor(sUrl)->GetSize();
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

// TODO: Implement driver functions from this point.
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
			return (void*)driver.CreateFileAccessor(sUrl)->OpenForReading()->GetHandle();
		case 'w':
			return (void*)driver.CreateFileAccessor(sUrl)->OpenForWriting()->GetHandle();
		case 'a':
			return (void*)driver.CreateFileAccessor(sUrl)->OpenForAppending()->GetHandle();
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
		driver.RetrieveFileStream(handle)->Close();
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
		return driver.RetrieveFileReader(FileStreamHandle(handle))->Read(dest, size, count);
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
		driver.RetrieveFileReader(handle)->Seek(offset, whence);
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
		return driver.RetrieveFileOutputStream(FileStreamHandle(handle))->Write(source, size, count);
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
		driver.RetrieveFileOutputStream(handle)->Flush();
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
		driver.CreateFileAccessor(sUrl)->Remove();
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
		driver.CreateFileAccessor(sUrl)->MkDir();
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
		driver.CreateFileAccessor(sUrl)->RmDir();
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
		return driver.CreateFileAccessor(sUrl)->GetFreeDiskSpace();
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
		driver.CreateFileAccessor(sSourceUrl)->CopyTo(sDestUrl);
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
		driver.CreateFileAccessor(sDestUrl)->CopyFrom(Azure::Core::Url(sSourceUrl));
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
	return driver.CreateFileAccessor(sUrl)->Exists() ? nTrue : nFalse;
}
