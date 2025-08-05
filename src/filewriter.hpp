#pragma once

namespace az
{
	class FileWriter;
}

#include <cstddef>
#include "fileoutputstream.hpp"

namespace az
{
	class FileWriter : public FileOutputStream
	{
	public:
		FileWriter();
		size_t Write(const void* source, size_t size, size_t count) override;
	};
}
