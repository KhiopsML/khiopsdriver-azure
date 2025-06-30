#include "env.hpp"
#include <spdlog/spdlog.h>
#include "string.hpp"

namespace az
{
	string GetEnvironmentVariableOrDefault(const string& varName, const string& defaultValue)
	{
		char* value = getenv(varName.c_str());

		if (value && strlen(value) > 0)
		{
			return value;
		}

		const string low_key = ToLower(varName);
		if (low_key.find("token") || low_key.find("password") || low_key.find("key") || low_key.find("secret"))
		{
			spdlog::debug("No {} specified, using **REDACTED** as default.", varName);
		}
		else
		{
			spdlog::debug("No {} specified, using '{}' as default.", varName, defaultValue);
		}

		return defaultValue;
	}
}
