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

	// TODO: Maybe this function should escape more things like pluses chars and the like
	regex RegexFromGlobbingPattern(const string& sGlobbingPattern)
	{
		return regex(
			regex_replace(
				regex_replace(
					regex_replace(
						sGlobbingPattern,
						regex("\\."),
						"\\."
					),
					regex("?"),
					"."),
				regex("*"),
				".*"
			)
		);
	}
}
}
