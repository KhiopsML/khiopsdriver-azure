#include "connstring.hpp"
#include <regex>

namespace az
{
	ParsingError::ParsingError(const char* message) :
		Error(message)
	{
	}

	Account ParseConnectionString(const string& connectionString)
	{
		smatch match;
		Account account;
		if (!regex_search(connectionString, match, regex("AccountName=[^;]+")))
		{
			throw ParsingError("Failed to parse connection string: account name not found");
		}
		account.name = match[0];
		if (!regex_search(connectionString, match, regex("AccountKey=[^;]+")))
		{
			throw ParsingError("Failed to parse connection string: account key not found");
		}
		account.key = match[0];
		return account;
	}
}
