#pragma once

namespace az
{
	class EmulatedBlobAccessor;
}

#include "fileaccessor.hpp"
#include "blobaccessor.hpp"
#include "emulatedfileaccessor.hpp"
#include <azure/core.hpp>
#include <azure/storage/blobs.hpp>

namespace az
{
	class EmulatedBlobAccessor : public BlobAccessor, public EmulatedFileAccessor
	{
	public:
		EmulatedBlobAccessor(const Azure::Core::Url& url);
		~EmulatedBlobAccessor();

	protected:
		string GetAccountName() const;
		string GetContainerName() const override;
		string GetObjectName() const override;
		string GetServiceUrl() const override;
		string GetContainerUrl() const override;
		Azure::Storage::Blobs::BlobServiceClient GetServiceClient() const override;
		Azure::Storage::Blobs::BlobContainerClient GetContainerClient() const override;
		Azure::Storage::Blobs::BlobClient GetBlobClient() const override;
		vector<string> UrlPathParts() const override;
		void CheckFileUrl() const override;
		void CheckDirUrl() const override;
	};
}
