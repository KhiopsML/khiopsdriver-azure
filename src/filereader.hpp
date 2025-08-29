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
		~FileReader();
		virtual size_t Read(void* dest, size_t nSize, size_t nCount) = 0;
		virtual void Seek(long long int nOffset, int nOrigin) = 0;

	protected:
		FileReader();
		size_t nCurrentPos;
	};
}
