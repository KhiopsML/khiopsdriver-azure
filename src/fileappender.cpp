#include "fileappender.hpp"

using namespace std;

namespace az
{
	FileAppender::FileAppender() :
		FileOutputStream()
	{
	}

	size_t FileAppender::Write(const void* source, size_t size, size_t count)
	{
		// TODO: Implement
		return 0;
	}
}
