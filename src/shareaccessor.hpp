#pragma once

namespace az
{
	class ShareAccessor;
}

#include "fileaccessor.hpp"
#include <azure/core.hpp>

namespace az
{
	using namespace std;

	class ShareAccessor : public FileAccessor
	{
	public:
		ShareAccessor(const Azure::Core::Url& url, bool bIsEmulatedStorage);

		bool Exists() const;
		size_t GetSize() const;
		FileStream Open(char mode) const;
		void Remove() const;
		void MkDir() const;
		void RmDir() const;
		size_t GetFreeDiskSpace() const;
		void CopyTo(const Azure::Core::Url& destUrl) const;
		void CopyFrom(const Azure::Core::Url& sourceUrl) const;

		~ShareAccessor();
	};
}
