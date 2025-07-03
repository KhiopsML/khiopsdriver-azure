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
		inline ParsingError(const string& sMessage):
			sMessage(sMessage)
		{
		}

		inline const char* what() const override
		{
			return sMessage.c_str();
		}

		string sMessage;
	};

	ConnectionString ParseConnectionString(const string& sConnectionString);

	bool operator==(const ConnectionString& a, const ConnectionString& b);
}
