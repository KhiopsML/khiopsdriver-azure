#pragma once

#include <string>
#include <regex>

namespace az
{
namespace globbing
{
	size_t FindGlobbingChar(const std::string& str);
	std::regex RegexFromGlobbingPattern(const std::string& sGlobbingPattern);
}
}
