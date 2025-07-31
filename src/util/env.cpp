#include "env.hpp"
#include <sstream>
#include <spdlog/spdlog.h>
#include "string.hpp"

using namespace std;

namespace az
{
	EnvironmentVariableNotFoundError::EnvironmentVariableNotFoundError(const std::string& sVarName) :
		sMessage((std::ostringstream() << "environment variable '" << sVarName << "' not found").str())
	{
	}

	const char* EnvironmentVariableNotFoundError::what() const noexcept
	{
		return sMessage.c_str();
	}

	string GetEnvironmentVariableOrThrow(const string& sVarName)
	{
		char* sValue = getenv(sVarName.c_str());
		if (!sValue)
		{
			throw EnvironmentVariableNotFoundError(sVarName);
		}
		return sValue;
	}

	string GetEnvironmentVariableOrDefault(const string& sVarName, const string& sDefaultValue)
	{
		char* sValue = getenv(sVarName.c_str());

		if (sValue && strlen(sValue) > 0)
		{
			return sValue;
		}

		string low_key = ToLower(sVarName);
		if (low_key.find("token") != string::npos || low_key.find("password") != string::npos || low_key.find("key") != string::npos || low_key.find("secret") != string::npos)
		{
			spdlog::debug("No {} specified, using **REDACTED** as default.", sVarName);
		}
		else
		{
			spdlog::debug("No {} specified, using '{}' as default.", sVarName, sDefaultValue);
		}

		return sDefaultValue;
	}
}
