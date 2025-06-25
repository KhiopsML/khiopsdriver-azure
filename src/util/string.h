#pragma once

#include <vector>
#include <string>

namespace az
{
	using namespace std;

	vector<string>&& Split(const string& str, char delim);
}
