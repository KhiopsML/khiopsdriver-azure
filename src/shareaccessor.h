#pragma once

namespace az
{
	class ShareAccessor;
}

#include "fileaccessor.h"
#include <azure/core.hpp>

namespace az
{
	using namespace std;

	class ShareAccessor : public FileAccessor
	{
	public:
		ShareAccessor(const Azure::Core::Url& url);

		bool Exists() const;
		size_t GetSize() const;
		FileStream Open(char mode) const;
	};
}
