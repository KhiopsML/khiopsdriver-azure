#pragma once

namespace az
{
	class FileOutputStream;
}

#include <cstddef>
#include "filestream.hpp"

namespace az
{
	class FileOutputStream : public FileStream
	{
	public:
		void Close() override;
		virtual size_t Write(const void* source, size_t size, size_t count) = 0;
		void Flush();

	protected:
		FileOutputStream();
	};
}
