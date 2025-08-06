#pragma once

namespace az
{
	class FileReader;
}

#include <cstddef>
#include "filestream.hpp"

namespace az
{
	class FileReader : public FileStream
	{
	public:
		virtual size_t Read(void* dest, size_t size, size_t count) = 0;
		virtual void Seek(long long int offset, int whence) = 0;

	protected:
		FileReader();
		size_t nCurrentPos;
	};
}
