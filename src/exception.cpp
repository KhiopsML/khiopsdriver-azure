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

	InvalidObjectPathError::InvalidObjectPathError(const string& sPath) :
		sMessage((ostringstream() << "invalid object path: " << sPath).str())
	{
	}

	const char* InvalidObjectPathError::what() const noexcept
	{
		return sMessage.c_str();
	}

	string FormatOperation(FileOperation operation)
	{
		switch (operation)
		{
		case FileOperation::MKDIR:
			return "making directory";
		case FileOperation::RMDIR:
			return "removing directory";
		default:
			throw invalid_argument((ostringstream() << "invalid FileOperation: " << (int)operation).str());
		}
	}

	InvalidOperationForFileError::InvalidOperationForFileError(FileOperation operation) :
		sMessage((ostringstream() << "files do not support this operation: " << FormatOperation(operation)).str())
	{
	}

	const char* InvalidOperationForFileError::what() const noexcept
	{
		return sMessage.c_str();
	}

	string FormatOperation(DirOperation operation)
	{
		switch (operation)
		{
		case DirOperation::GET_SIZE:
			return "getting size";
		case DirOperation::READ:
			return "reading";
		case DirOperation::WRITE:
			return "writing";
		case DirOperation::APPEND:
			return "appending";
		case DirOperation::REMOVE:
			return "removing (use driver_rmdir instead)";
		case DirOperation::COPY:
			return "copying";
		default:
			throw invalid_argument((ostringstream() << "invalid DirOperation: " << (int)operation).str());
		}
	}

	InvalidOperationForDirError::InvalidOperationForDirError(DirOperation operation):
		sMessage((ostringstream() << "directories do not support this operation: " << FormatOperation(operation)).str())
	{}

	const char* InvalidOperationForDirError::what() const noexcept
	{
		return sMessage.c_str();
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

	InvalidSeekOffsetError::InvalidSeekOffsetError(long long int nOffset, int nOrigin) :
		sMessage((ostringstream() << "tried to seek " << nOffset << " bytes from origin '" << nOrigin << "' which is outside the file").str())
	{
	}

	const char* InvalidSeekOffsetError::what() const noexcept
	{
		return sMessage.c_str();
	}

	FileStreamNotFoundError::FileStreamNotFoundError(const void* handle) :
		sMessage((ostringstream() << "file stream with handle '" << handle << "' not found").str())
	{
	}

	const char* FileStreamNotFoundError::what() const noexcept
	{
		return sMessage.c_str();
	}

	IntermediateDirNotFoundError::IntermediateDirNotFoundError(const string& sUrl) :
		sMessage((ostringstream() << "intermediate directory '" << sUrl << "' not found").str())
	{
	}

	const char* IntermediateDirNotFoundError::what() const noexcept
	{
		return sMessage.c_str();
	}

	DirAlreadyExistsError::DirAlreadyExistsError(const string& sUrl) :
		sMessage((ostringstream() << "directory '" << sUrl << "' already exists").str())
	{
	}

	const char* DirAlreadyExistsError::what() const noexcept
	{
		return sMessage.c_str();
	}

	CreationError::CreationError(const string& sUrl) :
		sMessage((ostringstream() << "failed to create " << sUrl).str())
	{
	}

	const char* CreationError::what() const noexcept
	{
		return sMessage.c_str();
	}
}
