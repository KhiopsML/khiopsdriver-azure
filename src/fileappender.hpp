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
		size_t Write(const void* source, size_t size, size_t count) override;
	};
}
