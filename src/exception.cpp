#include "exception.hpp"

namespace az
{
	Error::Error():
		exception()
	{
	}

	Error::Error(const char* message) :
		exception(message)
	{
	}
}
