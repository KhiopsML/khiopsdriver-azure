#include "connstring.hpp"
#include <regex>
#include <unordered_map>

using namespace std;

namespace az
{
	ParsingError::ParsingError(const std::string& sMessage) :
		sMessage(sMessage)
	{
	}

	const char* ParsingError::what() const noexcept
	{
		return sMessage.c_str();
	}

	// This is the default Azurite connection string, split in multiple lines for readability:
	// AccountName=devstoreaccount1;
	// AccountKey=Eby8vdM02xNOcqFlqUwJPLlmEtlCDXJ1OUzFT50uSRZ6IFsuFq2UVErCz4I6tq/K1SZFPTOtr/KBHBeksoGMGw==;
	// DefaultEndpointsProtocol=http;
	// BlobEndpoint=http://127.0.0.1:10000/devstoreaccount1;
	// QueueEndpoint=http://127.0.0.1:10001/devstoreaccount1;
	// TableEndpoint=http://127.0.0.1:10002/devstoreaccount1;
	ConnectionString ParseConnectionString(const string& sConnectionString)
	{
		smatch match;
		if (!regex_match(sConnectionString, match, regex("(?:[^=]+=[^;]+;)+")))
		{
			throw ParsingError("ill-formed connection string");
		}
		regex kvRegex("([^=]+)=([^;]+);");
		sregex_iterator begin(sConnectionString.begin(), sConnectionString.end(), kvRegex);
		sregex_iterator end;
		unordered_map<string, string> kvPairs;
		for (sregex_iterator it = begin; it != end; it++)
		{
			kvPairs[(*it)[1]] = (*it)[2];
		}
		auto accountNameIt = kvPairs.find("AccountName");
		if (accountNameIt == kvPairs.end())
		{
			throw ParsingError("connection string is missing AccountName");
		}
		auto accountKeyIt = kvPairs.find("AccountKey");
		if (accountKeyIt == kvPairs.end())
		{
			throw ParsingError("connection string is missing AccountKey");
		}
		auto blobEndpointIt = kvPairs.find("BlobEndpoint");
		if (blobEndpointIt == kvPairs.end())
		{
			throw ParsingError("connection string is missing BlobEndpoint");
		}
		return ConnectionString
		{
			accountNameIt->second,
			accountKeyIt->second,
			Azure::Core::Url(blobEndpointIt->second)
		};
	}

	bool operator==(const ConnectionString& a, const ConnectionString& b)
	{
		return a.sAccountName == b.sAccountName
			&& a.sAccountKey == b.sAccountKey
			&& a.blobEndpoint.GetAbsoluteUrl() == b.blobEndpoint.GetAbsoluteUrl();
	}
}
