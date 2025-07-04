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
}

#include <exception>
#include <string>
#include <sstream>

namespace az
{
	using namespace std;

	class Error: public exception
	{
	};

	class InvalidDomainError : public Error
	{
	public:
		inline InvalidDomainError(const string& sDomain):
			sMessage((ostringstream() << "invalid domain: " << sDomain).str())
		{
		}

		inline const char* what() const override
		{
			return sMessage.c_str();
		}

	private:
		string sMessage;
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
		inline InvalidUrlError(const string& sUrl):
			sMessage((ostringstream() << "invalid URL: " << sUrl).str())
		{
		}

		inline const char* what() const override
		{
			return sMessage.c_str();
		}

	private:
		string sMessage;
	};

	class InvalidFileUrlPathError : public Error
	{
	public:
		inline InvalidFileUrlPathError(const string& sPath):
			sMessage((ostringstream() << "invalid file URL path: " << sPath).str())
		{
		}

		inline const char* what() const override
		{
			return sMessage.c_str();
		}

	private:
		string sMessage;
	};

	class InvalidDirUrlPathError : public Error
	{
	public:
		inline InvalidDirUrlPathError(const string& sPath):
			sMessage((ostringstream() << "invalid directory URL path: " << sPath).str())
		{
		}

		inline const char* what() const override
		{
			return sMessage.c_str();
		}

	private:
		string sMessage;
	};
}
