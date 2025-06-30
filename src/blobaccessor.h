#pragma once

namespace az
{
	class BlobAccessor;
}

#include "fileaccessor.h"
#include <azure/core.hpp>

namespace az
{
	using namespace std;

	class BlobAccessor : public FileAccessor
	{
	public:
		BlobAccessor(const Azure::Core::Url& url);

		bool Exists() const;
		size_t GetSize() const;
		FileStream Open(char mode) const;
	};
}
