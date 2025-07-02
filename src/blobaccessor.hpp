#pragma once

namespace az
{
	class BlobAccessor;
}

#include "fileaccessor.hpp"
#include <azure/core.hpp>
#include <azure/storage/blobs.hpp>

namespace az
{
	class BlobAccessor : public FileAccessor
	{
	public:
		virtual ~BlobAccessor() = 0;

		bool Exists() const;
		size_t GetSize() const;
		FileStream Open(char mode) const;
		void Remove() const;
		void MkDir() const;
		void RmDir() const;
		size_t GetFreeDiskSpace() const;
		void CopyTo(const Azure::Core::Url& destUrl) const;
		void CopyFrom(const Azure::Core::Url& sourceUrl) const;

	protected:
		BlobAccessor(const Azure::Core::Url& url);

		virtual Azure::Storage::Blobs::BlobClient GetBlobClient() const = 0;
	};
}
