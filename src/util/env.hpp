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
		EnvironmentVariableNotFoundError(const std::string& sVarName);

		virtual const char* what() const noexcept override;

	private:
		std::string sMessage;
	};

	std::string GetEnvironmentVariableOrThrow(const std::string& sVarName);
	std::string GetEnvironmentVariableOrDefault(const std::string& sVarName, const std::string& sDefaultValue);
}
