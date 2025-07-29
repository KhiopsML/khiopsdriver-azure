#include "glob.hpp"

using namespace std;

namespace az
{
namespace globbing
{
	size_t FindGlobbingChar(const string& str)
	{
		smatch match;
		return regex_search(str, match, regex("[^\\]([*?![^])", regex_constants::extended)) ? match.position(1) : string::npos;
	}
}
}
