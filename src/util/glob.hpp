#pragma once

#include <string>
#include <regex>

namespace az
{
using namespace std;
namespace globbing
{
	size_t FindGlobbingChar(const string& str);
	regex RegexFromGlobbingPattern(const string& sGlobbingPattern);
}
}
