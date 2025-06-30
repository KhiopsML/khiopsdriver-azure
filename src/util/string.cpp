#include "string.hpp"
#include <sstream>
#include <algorithm>

namespace az
{
	using namespace std;

	vector<string>&& Split(const string& str, char delim)
	{
		vector<string> fragments;
		istringstream iss(str);
		string fragment;
		while (getline(iss, fragment, delim))
		{
			fragments.push_back(move(fragment));
		}
		return move(fragments);
	}

	bool EndsWith(const std::string& str, const std::string& suffix)
	{
		size_t strLen = str.length();
		size_t suffixLen = str.length();
		return suffixLen <= strLen && !str.compare(strLen - suffixLen, suffixLen, suffix);
	}

	string&& ToLower(const string& str)
	{
		string lower(str.length(), '\0');
		transform(str.begin(), str.end(), lower.begin(), tolower);
		return move(lower);
	}
}
