#pragma once

#include <vector>
#include <string>

namespace az
{
	using namespace std;

	vector<string> Split(const string& str, char delim, long long int nMaxSplits = -1);

	bool StartsWith(const string& str, const string& prefix);
	bool EndsWith(const string& str, const string& suffix);

	string ToLower(const string& str);
}
