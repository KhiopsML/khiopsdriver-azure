#pragma once

#include <vector>
#include <string>
#include <sstream>

namespace az
{
	using namespace std;

	vector<string>&& Split(const string& str, char delim);
}
