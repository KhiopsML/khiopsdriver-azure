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
	struct ConnectionString
	{
		std::string sAccountName;
		std::string sAccountKey;
		Azure::Core::Url blobEndpoint;
	};

	class ParsingError : public Error
	{
	public:
		inline ParsingError(const std::string& sMessage):
			sMessage(sMessage)
		{
		}

		virtual const char* what() const noexcept
		{
			return sMessage.c_str();
		}

		std::string sMessage;
	};

	ConnectionString ParseConnectionString(const std::string& sConnectionString);

	bool operator==(const ConnectionString& a, const ConnectionString& b);
}
