#include "string.h"

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
}
