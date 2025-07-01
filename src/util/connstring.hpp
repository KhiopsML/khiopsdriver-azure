#pragma once

namespace az
{
	struct Account;
	class ParsingError;
}

#include <string>
#include "../exception.hpp"

namespace az
{
	using namespace std;

	struct Account
	{
		string name;
		string key;
	};

	class ParsingError : public Error
	{
	public:
		ParsingError(const char* message);
	};

	Account ParseConnectionString(const string& connectionString);
}
