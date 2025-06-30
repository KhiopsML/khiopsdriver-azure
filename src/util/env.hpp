#pragma once

#include <string>

using namespace std;

namespace az
{
	string GetEnvironmentVariableOrDefault(const string& variable_name, const string& default_value);
}
