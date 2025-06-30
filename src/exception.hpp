#pragma once

namespace az
{
	class Exception;
	class InvalidDomainException;
}

#include <exception>

namespace az
{
	using namespace std;

	class Exception : public exception
	{
	};

	class InvalidDomainException : public Exception
	{
	};

	class NotConnectedException : public Exception
	{
	};
}
