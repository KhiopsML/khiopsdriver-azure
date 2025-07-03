#pragma once

namespace az
{
	class Error;
	class InvalidDomainError;
	class NotConnectedError;
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
}
