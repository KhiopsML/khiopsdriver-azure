#pragma once

#include "exception.hpp"
#include <sstream>

using namespace std;

namespace az
{
	NullArgError::NullArgError(const char* sFuncname, const char* sArgname) :
		sMessage((ostringstream() << "error passing null pointer as '" << sArgname << "' argument to function '" << sFuncname << "'").str())
	{
	}

	const char* NullArgError::what() const noexcept
	{
		return sMessage.c_str();
	}

	InvalidDomainError::InvalidDomainError(const string& sDomain) :
		sMessage((ostringstream() << "invalid domain: " << sDomain).str())
	{
	}

	const char* InvalidDomainError::what() const noexcept
	{
		return sMessage.c_str();
	}

	const char* NotConnectedError::what() const noexcept
	{
		return "not connected";
	}

	const char* IncompatibleConnectionStringError::what() const noexcept
	{
		return "connection string is not valid for the provided URL";
	}

	const char* NetworkError::what() const noexcept
	{
		return "failed to communicate with the storage server";
	}

	InvalidUrlError::InvalidUrlError(const string& sUrl) :
		sMessage((stringstream() << "invalid URL: " << sUrl).str())
	{
	}

	const char* InvalidUrlError::what() const noexcept
	{
		return sMessage.c_str();
	}

	InvalidFileUrlPathError::InvalidFileUrlPathError(const string& sPath) :
		sMessage((ostringstream() << "invalid file URL path: " << sPath).str())
	{
	}

	const char* InvalidFileUrlPathError::what() const noexcept
	{
		return sMessage.c_str();
	}

	InvalidDirUrlPathError::InvalidDirUrlPathError(const string& sPath) :
		sMessage((ostringstream() << "invalid directory URL path: " << sPath).str())
	{
	}

	const char* InvalidDirUrlPathError::what() const noexcept
	{
		return sMessage.c_str();
	}

	const char* GettingSizeOfDirError::what() const noexcept
	{
		return "trying to get size of directory (invalid operation)";
	}

	NoFileError::NoFileError(const string& sUrl) :
		sMessage((ostringstream() << "no file exists at URL " << sUrl).str())
	{
	}

	const char* NoFileError::what() const noexcept
	{
		return sMessage.c_str();
	}

	DeletionError::DeletionError(const string& sUrl) :
		sMessage((ostringstream() << "failed to delete " << sUrl).str())
	{
	}

	const char* DeletionError::what() const noexcept
	{
		return sMessage.c_str();
	}

	InvalidFileStreamModeError::InvalidFileStreamModeError(const string& sUrl, char mode) :
		sMessage((ostringstream() << "tried to open file " << sUrl << " with invalid mode " << mode).str())
	{
	}

	const char* InvalidFileStreamModeError::what() const noexcept
	{
		return sMessage.c_str();
	}

	InvalidSeekOriginError::InvalidSeekOriginError(int nOrigin) :
		sMessage((ostringstream() << "tried to seek from invalid origin '" << nOrigin << "'").str())
	{
	}

	const char* InvalidSeekOriginError::what() const noexcept
	{
		return sMessage.c_str();
	}

	InvalidSeekOffsetError::InvalidSeekOffsetError(int nOffset, int nOrigin) :
		sMessage((ostringstream() << "tried to seek " << nOffset << " bytes from origin '" << nOrigin << "' which is outside the file").str())
	{
	}

	const char* InvalidSeekOffsetError::what() const noexcept
	{
		return sMessage.c_str();
	}

	FileStreamNotFoundError::FileStreamNotFoundError(const FileStreamHandle& handle) :
		sMessage((ostringstream() << "file stream with handle '" << (void*)handle << "' not found").str())
	{
	}

	const char* FileStreamNotFoundError::what() const noexcept
	{
		return sMessage.c_str();
	}
}
