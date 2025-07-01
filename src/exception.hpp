#pragma once

namespace az
{
	class Error;
	class InvalidDomainError;
	class NotConnectedError;
}

#include <exception>

namespace az
{
	using namespace std;

	class Error : public exception
	{
	public:
		Error();
		Error(const char* message);
	};

	class InvalidDomainError : public Error
	{
	};

	class NotConnectedError : public Error
	{
	};
}
