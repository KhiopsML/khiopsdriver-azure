#pragma once

namespace az
{
	class Error;
	class InvalidDomainError;
	class NotConnectedError;
	class IncompatibleConnectionStringError;
	class NetworkError;
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
}
