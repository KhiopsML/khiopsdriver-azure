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
	};
}
