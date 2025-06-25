#pragma once

#include <vector>
#include <string>

namespace az
{
	using namespace std;

	vector<string>&& Split(const string& str, char delim);

	bool EndsWith(const std::string& str, const std::string& suffix);
}
