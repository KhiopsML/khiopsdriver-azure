#pragma once

namespace az
{
	class FileAccessor;
}

#include <string>
#include "filestream.h"

namespace az
{
	using namespace std;

	class FileAccessor
	{
	public:
		virtual bool Exists() const = 0;
		virtual size_t GetSize() const = 0;
		virtual FileStream Open() const = 0;
	};
}
