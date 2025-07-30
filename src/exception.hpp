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
			return "failed to communicate to the storage server";
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
}
