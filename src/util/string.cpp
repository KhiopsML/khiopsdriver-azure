#include "string.hpp"
#include <sstream>
#include <algorithm>

using namespace std;

namespace az
{
	vector<string> Split(const string& str, char delim, long long int nMaxSplits, bool bRemoveEmpty)
	{
		size_t nStrLen = str.length();
		vector<string> fragments;
		size_t nOffset = 0;
		size_t nDelimPos;
		string sFragment;
		for (size_t nSplits = 0; nMaxSplits == -1 || nSplits <= static_cast<size_t>(nMaxSplits); nSplits++)
		{
			nDelimPos = nSplits == static_cast<size_t>(nMaxSplits) ? string::npos : str.find(delim, nOffset);
			sFragment = nOffset == nStrLen ? "" : str.substr(nOffset, nDelimPos - nOffset);
			if (!sFragment.empty() || !bRemoveEmpty)
			{
				fragments.push_back(move(sFragment));
			}
			if (nDelimPos == string::npos)
			{
				break;
			}
			nOffset = nDelimPos + 1;
		}
		return fragments;
	}

	bool StartsWith(const string& str, const string& prefix)
	{
		size_t strLen = str.length();
		size_t prefixLen = prefix.length();
		return prefixLen <= strLen && !str.compare(0, prefixLen, prefix);
	}

	bool EndsWith(const std::string& str, const std::string& suffix)
	{
		size_t strLen = str.length();
		size_t suffixLen = suffix.length();
		return suffixLen <= strLen && !str.compare(strLen - suffixLen, suffixLen, suffix);
	}

	string ToLower(const string& str)
	{
		string lower(str.length(), '\0');
		transform(str.begin(), str.end(), lower.begin(), [](char ch) { return tolower(ch); });
		return lower;
	}
}
