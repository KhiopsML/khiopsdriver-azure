// File:        globmatch.hpp, derived from orginal match.cpp
// Author:      Robert A. van Engelen, engelen@genivia.com
// Date:        August 5, 2019
// License:     The Code Project Open License (CPOL)
//              https://www.codeproject.com/info/cpol10.aspx

#pragma once

#include <string>

namespace az
{
namespace globbing
{
	// returns TRUE if text string matches gitignore-style glob pattern. match is case sensitive
	bool GitignoreGlobMatch(const std::string& text, const std::string& glob);
}
}
