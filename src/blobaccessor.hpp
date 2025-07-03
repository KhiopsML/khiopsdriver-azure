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

		bool Exists() const override;
		size_t GetSize() const override;
		FileStream Open(char mode) const override;
		void Remove() const override;
		void MkDir() const override;
		void RmDir() const override;
		size_t GetFreeDiskSpace() const override;
		void CopyTo(const Azure::Core::Url& destUrl) const override;
		void CopyFrom(const Azure::Core::Url& sourceUrl) const override;

	protected:
		BlobAccessor(const Azure::Core::Url& url);

		virtual Azure::Storage::Blobs::BlobServiceClient GetServiceClient() const = 0;
		virtual Azure::Storage::Blobs::BlobContainerClient GetContainerClient() const = 0;
		virtual Azure::Storage::Blobs::BlobClient GetBlobClient() const = 0;
	};
}
