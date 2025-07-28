#pragma once

#include <vector>
#include <string>

namespace az
{
	std::vector<std::string> Split(const std::string& str, char delim, long long int nMaxSplits = -1);

	bool StartsWith(const std::string& str, const std::string& prefix);
	bool EndsWith(const std::string& str, const std::string& suffix);

	std::string ToLower(const std::string& str);
}
