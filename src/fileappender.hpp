#pragma once

namespace az
{
	class FileAppender;
}

#include <cstddef>
#include "fileoutputstream.hpp"

namespace az
{
	class FileAppender : public FileOutputStream
	{
	public:
		FileAppender();
		virtual size_t Write(const void* source, size_t nSize, size_t nCount) = 0;
	};
}
