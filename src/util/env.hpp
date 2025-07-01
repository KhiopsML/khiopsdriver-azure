#pragma once

namespace az
{
	class EnvironmentVariableNotFoundError;
}

#include <string>
#include "../exception.hpp"

using namespace std;

namespace az
{
	class EnvironmentVariableNotFoundError : Error
	{
	public:
		EnvironmentVariableNotFoundError(const string& sVarName);
		const string& getVarName() const;

	private:
		string sVarName;
	};

	string GetEnvironmentVariableOrThrow(const string& sVarName);
	string GetEnvironmentVariableOrDefault(const string& sVarName, const string& sDefaultValue);
}
