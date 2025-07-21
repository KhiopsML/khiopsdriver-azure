#include "glob.hpp"

namespace az
{
namespace globbing
{
	size_t FindGlobbingChar(const string& str)
	{
		smatch match;
		return regex_search(str, regex("[^\\][*?![^]")) ? match.position() : string::npos;
	}

	regex RegexFromGlobbingPattern(const string& sGlobbingPattern)
	{
		return regex(
			regex_replace(
				regex_replace(sGlobbingPattern, regex("?"), "."),
				regex("*"),
				".*"
			)
		);
	}
}
}
