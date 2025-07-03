#pragma once

namespace az
{
	class EnvironmentVariableNotFoundError;
}

#include <string>
#include <sstream>
#include "../exception.hpp"

using namespace std;

namespace az
{
	class EnvironmentVariableNotFoundError : public Error
	{
	public:
		inline EnvironmentVariableNotFoundError(const string& sVarName):
			sMessage((ostringstream() << "environment variable '" << sVarName << "' not found").str())
		{
		}

		inline const char* what() const override
		{
			return sMessage.c_str();
		}

	private:
		string sMessage;
	};

	string GetEnvironmentVariableOrThrow(const string& sVarName);
	string GetEnvironmentVariableOrDefault(const string& sVarName, const string& sDefaultValue);
}
