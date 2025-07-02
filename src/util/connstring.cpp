#include "connstring.hpp"
#include <regex>
#include <unordered_map>

namespace az
{
	ParsingError::ParsingError(const char* sMessage) :
		Error(sMessage)
	{
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
		if (!regex_match(sConnectionString, smatch(), regex("(?:[^=]+=[^;]+;)+")))
		{
			throw ParsingError("Ill-formed connection string");
		}
		regex kvRegex("([^=]+)=([^;]+);");
		sregex_iterator begin(sConnectionString.begin(), sConnectionString.end(), kvRegex);
		sregex_iterator end;
		unordered_map<string, string> kvPairs;
		for (sregex_iterator it = begin; it != end; it++)
		{
			kvPairs[(*it)[1]] = (*it)[2];
		}
		try
		{
			return ConnectionString
			{
				kvPairs.at("AccountName"),
				kvPairs.at("AccountKey"),
				Azure::Core::Url(kvPairs.at("BlobEndpoint"))
			};
		}
		catch (const out_of_range&)
		{
			throw ParsingError("Connection string is missing one of 'AccountName', 'AccountKey' or 'BlobEndpoint'");
		}
	}
}
