#pragma once

namespace az
{
	class Error;
	class InvalidDomainError;
	class NotConnectedError;
	class IncompatibleConnectionStringError;
	class NetworkError;
	class InvalidUrlError;
	class InvalidFileUrlPathError;
	class InvalidDirUrlPathError;
	class GettingSizeOfDirError;
	class NoFileError;
	class DeletionError;
	class InvalidFileStreamModeError;
	class InvalidSeekOriginError;
	class InvalidSeekOffsetError;
}

#include <exception>
#include <string>
#include <sstream>

namespace az
{
	class Error: public std::exception
	{
	};

	class NullArgError : public Error
	{
	public:
		NullArgError(const char* sFuncname, const char* sArgname):
			sMessage((std::ostringstream() << "error passing null pointer as '" << sArgname << "' argument to function '" << sFuncname << "'").str())
		{
		}

		virtual const char* what() const noexcept override
		{
			return sMessage.c_str();
		}

	private:
		std::string sMessage;
	};

	class InvalidDomainError : public Error
	{
	public:
		InvalidDomainError(const std::string& sDomain):
			sMessage((std::ostringstream() << "invalid domain: " << sDomain).str())
		{
		}

		virtual const char* what() const noexcept override
		{
			return sMessage.c_str();
		}

	private:
		std::string sMessage;
	};

	class NotConnectedError : public Error
	{
	public:
		virtual const char* what() const noexcept override
		{
			return "not connected";
		}
	};

	class IncompatibleConnectionStringError : public Error
	{
	public:
		virtual const char* what() const noexcept override
		{
			return "connection string is not valid for the provided URL";
		}
	};

	class NetworkError : public Error
	{
	public:
		virtual const char* what() const noexcept override
		{
			return "failed to communicate with the storage server";
		}
	};

	class InvalidUrlError : public Error
	{
	public:
		InvalidUrlError(const std::string& sUrl):
			sMessage((std::ostringstream() << "invalid URL: " << sUrl).str())
		{
		}

		virtual const char* what() const noexcept override
		{
			return sMessage.c_str();
		}

	private:
		std::string sMessage;
	};

	class InvalidFileUrlPathError : public Error
	{
	public:
		InvalidFileUrlPathError(const std::string& sPath):
			sMessage((std::ostringstream() << "invalid file URL path: " << sPath).str())
		{
		}

		virtual const char* what() const noexcept override
		{
			return sMessage.c_str();
		}

	private:
		std::string sMessage;
	};

	class InvalidDirUrlPathError : public Error
	{
	public:
		InvalidDirUrlPathError(const std::string& sPath):
			sMessage((std::ostringstream() << "invalid directory URL path: " << sPath).str())
		{
		}

		virtual const char* what() const noexcept override
		{
			return sMessage.c_str();
		}

	private:
		std::string sMessage;
	};

	class GettingSizeOfDirError : public Error
	{
	public:
		virtual const char* what() const noexcept override
		{
			return "trying to get size of directory (invalid operation)";
		}
	};

	class NoFileError : public Error
	{
	public:
		NoFileError(const std::string& sUrl) :
			sMessage((std::ostringstream() << "no file exists at URL " << sUrl).str())
		{
		}

		virtual const char* what() const noexcept override
		{
			return sMessage.c_str();
		}

	private:
		std::string sMessage;
	};

	class DeletionError : public Error
	{
	public:
		DeletionError(const std::string& sUrl) :
			sMessage((std::ostringstream() << "failed to delete " << sUrl).str())
		{
		}

		virtual const char* what() const noexcept override
		{
			return sMessage.c_str();
		}

	private:
		std::string sMessage;
	};

	class InvalidFileStreamModeError : public Error
	{
	public:
		InvalidFileStreamModeError(const std::string& sUrl, char mode) :
			sMessage((std::ostringstream() << "tried to open file " << sUrl << " with invalid mode " << mode).str())
		{
		}

		virtual const char* what() const noexcept override
		{
			return sMessage.c_str();
		}

	private:
		std::string sMessage;
	};

	class InvalidSeekOriginError : public Error
	{
	public:
		InvalidSeekOriginError(int nOrigin) :
			sMessage((std::ostringstream() << "tried to seek from invalid origin '" << nOrigin << "'").str())
		{
		}

		virtual const char* what() const noexcept override
		{
			return sMessage.c_str();
		}

	private:
		std::string sMessage;
	};

	class InvalidSeekOffsetError : public Error
	{
	public:
		InvalidSeekOffsetError(int nOffset, int nOrigin) :
			sMessage((std::ostringstream() << "tried to seek " << nOffset << " bytes from origin '" << nOrigin << "' which is outside the file").str())
		{
		}

		virtual const char* what() const noexcept override
		{
			return sMessage.c_str();
		}

	private:
		std::string sMessage;
	};
}
