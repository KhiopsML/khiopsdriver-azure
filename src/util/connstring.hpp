#pragma once

namespace az
{
	struct ConnectionString;
	class ParsingError;
}

#include <string>
#include <azure/core/url.hpp>
#include "../exception.hpp"

namespace az
{
	using namespace std;

	struct ConnectionString
	{
		string sAccountName;
		string sAccountKey;
		Azure::Core::Url blobEndpoint;
	};

	class ParsingError : public Error
	{
	public:
		ParsingError(const char* sMessage);
	};

	ConnectionString ParseConnectionString(const string& sConnectionString);
}
