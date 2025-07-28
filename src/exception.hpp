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
		inline NullArgError(const char* sFuncname, const char* sArgname):
			sMessage((std::ostringstream() << "error passing null pointer as '" << sArgname << "' argument to function '" << sFuncname << "'").str())
		{
		}

		inline const char* what() const override
		{
			return sMessage.c_str();
		}

	private:
		std::string sMessage;
	};

	class InvalidDomainError : public Error
	{
	public:
		inline InvalidDomainError(const std::string& sDomain):
			sMessage((std::ostringstream() << "invalid domain: " << sDomain).str())
		{
		}

		inline const char* what() const override
		{
			return sMessage.c_str();
		}

	private:
		std::string sMessage;
	};

	class NotConnectedError : public Error
	{
	public:
		inline const char* what() const override
		{
			return "not connected";
		}
	};

	class IncompatibleConnectionStringError : public Error
	{
	public:
		inline const char* what() const override
		{
			return "connection string is not valid for the provided URL";
		}
	};

	class NetworkError : public Error
	{
	public:
		inline const char* what() const override
		{
			return "failed to communicate to the storage server";
		}
	};

	class InvalidUrlError : public Error
	{
	public:
		inline InvalidUrlError(const std::string& sUrl):
			sMessage((std::ostringstream() << "invalid URL: " << sUrl).str())
		{
		}

		inline const char* what() const override
		{
			return sMessage.c_str();
		}

	private:
		std::string sMessage;
	};

	class InvalidFileUrlPathError : public Error
	{
	public:
		inline InvalidFileUrlPathError(const std::string& sPath):
			sMessage((std::ostringstream() << "invalid file URL path: " << sPath).str())
		{
		}

		inline const char* what() const override
		{
			return sMessage.c_str();
		}

	private:
		std::string sMessage;
	};

	class InvalidDirUrlPathError : public Error
	{
	public:
		inline InvalidDirUrlPathError(const std::string& sPath):
			sMessage((std::ostringstream() << "invalid directory URL path: " << sPath).str())
		{
		}

		inline const char* what() const override
		{
			return sMessage.c_str();
		}

	private:
		std::string sMessage;
	};

	class GettingSizeOfDirError : public Error
	{
	public:
		inline const char* what() const override
		{
			return "trying to get size of directory (invalid operation)";
		}
	};
}
