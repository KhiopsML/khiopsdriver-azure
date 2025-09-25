// Exceptions thrown by the driver. They are caught by the C-interface so they do not propagate to user code.

#pragma once

#include <exception>
#include <string>
#include <sstream>

namespace az
{
	class Error: public std::exception
	{
	public:
		inline Error(std::string sMessage) : sMessage(sMessage) {}
		inline virtual const char* what() const noexcept override { return sMessage.c_str(); };

	protected:
		std::string sMessage;
	};

	class NullArgError : public Error
	{
	public:
		inline NullArgError(const char* sFuncname, const char* sArgname) :
			Error((std::ostringstream() << "error passing null pointer as '" << sArgname << "' argument to function '" << sFuncname << "'").str())
		{}
	};

	class InvalidDomainError : public Error
	{
	public: inline InvalidDomainError(const std::string& sDomain): Error((std::ostringstream() << "invalid domain: " << sDomain).str()) {}
	};

	class NotConnectedError : public Error
	{
	public: inline NotConnectedError() : Error("not connected") {}
	};

	class IncompatibleConnectionStringError : public Error
	{
	public: inline IncompatibleConnectionStringError() : Error("connection string is not valid for the provided URL") {}
	};

	class NetworkError : public Error
	{
	public: inline NetworkError() : Error("failed to communicate with the storage server") {}
	};

	class InvalidUrlError : public Error
	{
	public: inline InvalidUrlError(const std::string& sUrl) : Error((std::stringstream() << "invalid URL: " << sUrl).str()) {}
	};

	class InvalidObjectPathError : public Error
	{
	public: inline InvalidObjectPathError(const std::string& sPath) : Error((std::ostringstream() << "invalid object path: " << sPath).str()) {}
	};

	enum class FileOperation { MKDIR, RMDIR };

	inline std::string FormatOperation(FileOperation operation)
	{
		switch (operation)
		{
		case FileOperation::MKDIR:
			return "making directory";
		case FileOperation::RMDIR:
			return "removing directory";
		default:
			throw std::invalid_argument((std::ostringstream() << "invalid FileOperation: " << (int)operation).str());
		}
	}

	class InvalidOperationForFileError : public Error
	{
	public:
		inline InvalidOperationForFileError(FileOperation operation) :
			Error((std::ostringstream() << "files do not support this operation: " << FormatOperation(operation)).str())
		{}
	};

	enum class DirOperation { GET_SIZE, READ, WRITE, APPEND, REMOVE, COPY };

	inline std::string FormatOperation(DirOperation operation)
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
			throw std::invalid_argument((std::ostringstream() << "invalid DirOperation: " << (int)operation).str());
		}
	}

	class InvalidOperationForDirError : public Error
	{
	public:
		inline InvalidOperationForDirError(DirOperation operation) :
			Error((std::ostringstream() << "directories do not support this operation: " << FormatOperation(operation)).str())
		{}
	};

	class NoFileError : public Error
	{
	public: inline NoFileError(const std::string& sUrl) : Error((std::ostringstream() << "no file exists at URL " << sUrl).str()) {}
	};

	class DeletionError : public Error
	{
	public: inline DeletionError(const std::string& sUrl) : Error((std::ostringstream() << "failed to delete " << sUrl).str()) {}
	};

	class InvalidFileStreamModeError : public Error
	{
	public:
		inline InvalidFileStreamModeError(const std::string& sUrl, char mode) :
			Error((std::ostringstream() << "tried to open file " << sUrl << " with invalid mode " << mode).str())
		{}
	};

	class InvalidSeekOriginError : public Error
	{
	public:
		inline InvalidSeekOriginError(int nOrigin) :
			Error((std::ostringstream() << "tried to seek from invalid origin '" << nOrigin << "'").str())
		{}
	};

	class InvalidSeekOffsetError : public Error
	{
	public:
		inline InvalidSeekOffsetError(long long int nOffset, int nOrigin) :
			Error((std::ostringstream() << "tried to seek " << nOffset << " bytes from origin '" << nOrigin << "' which is outside the file").str())
		{}
	};

	class FileStreamNotFoundError : public Error
	{
	public: inline FileStreamNotFoundError(const void* handle) : Error((std::ostringstream() << "file stream with handle '" << handle << "' not found").str()) {}
	};

	class IntermediateDirNotFoundError : public Error
	{
	public: inline IntermediateDirNotFoundError(const std::string& sUrl) : Error((std::ostringstream() << "intermediate directory '" << sUrl << "' not found").str()) {}
	};

	class DirAlreadyExistsError : public Error
	{
	public: inline DirAlreadyExistsError(const std::string& sUrl) : Error((std::ostringstream() << "directory '" << sUrl << "' already exists").str()) {}
	};

	class CreationError : public Error
	{
	public: inline CreationError(const std::string& sUrl) : Error((std::ostringstream() << "failed to create " << sUrl).str()) {}
	};

	class ReadAtEOFError : public Error
	{
	public: inline ReadAtEOFError() : Error("cannot read after end of file") {}
	};

	class ReadingUpdatedFileError : public Error
	{
	public: inline ReadingUpdatedFileError() : Error("the file has been updated during the reading") {}
	};
}
