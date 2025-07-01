#pragma once

namespace az
{
	class BlobAccessor;
}

#include "fileaccessor.hpp"
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
		void Remove() const;
		void MkDir() const;
		void RmDir() const;
		size_t GetFreeDiskSpace() const;
		void CopyTo(const Azure::Core::Url& destUrl) const;
		void CopyFrom(const Azure::Core::Url& sourceUrl) const;

		~BlobAccessor();

	protected:
		Azure::Storage::Blobs::BlobClient GetBlobClient() const;
	};
}
