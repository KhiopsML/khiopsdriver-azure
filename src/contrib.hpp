// Third-party code

#pragma once

#include <string>

namespace az
{
	namespace util
	{
		namespace glob
		{
			// File:        From orginal match.cpp
			// Author:      Robert A. van Engelen, engelen@genivia.com
			// Date:        August 5, 2019
			// License:     The Code Project Open License (CPOL)
			//              https://www.codeproject.com/info/cpol10.aspx
			// returns TRUE if text string matches gitignore-style glob pattern. match is case sensitive
			bool GitignoreGlobMatch(const std::string& text, const std::string& glob);
		}
	}
}
