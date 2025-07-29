#pragma once

namespace az
{
	class EnvironmentVariableNotFoundError;
}

#include <string>
#include <sstream>
#include "../exception.hpp"

namespace az
{
	class EnvironmentVariableNotFoundError : public Error
	{
	public:
		inline EnvironmentVariableNotFoundError(const std::string& sVarName):
			sMessage((std::ostringstream() << "environment variable '" << sVarName << "' not found").str())
		{
		}

		virtual const char* what() const noexcept
		{
			return sMessage.c_str();
		}

	private:
		std::string sMessage;
	};

	std::string GetEnvironmentVariableOrThrow(const std::string& sVarName);
	std::string GetEnvironmentVariableOrDefault(const std::string& sVarName, const std::string& sDefaultValue);
}
