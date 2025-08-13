#include "fileappender.hpp"

using namespace std;

namespace az
{
	FileAppender::FileAppender() :
		FileOutputStream()
	{
	}

	size_t FileAppender::Write(const void* source, size_t nSize, size_t nCount)
	{
		// TODO: Implement
		return 0;
	}
}
