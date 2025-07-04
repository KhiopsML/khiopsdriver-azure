#pragma once

namespace az
{
	class CloudBlobAccessor;
}

#include "fileaccessor.hpp"
#include "blobaccessor.hpp"
#include "cloudfileaccessor.hpp"
#include <azure/core.hpp>
#include <azure/storage/blobs.hpp>

namespace az
{
	class CloudBlobAccessor : public BlobAccessor, public CloudFileAccessor
	{
	public:
		CloudBlobAccessor(const Azure::Core::Url& url);
		~CloudBlobAccessor();

	protected:
		string GetServiceUrl() const override;
		string GetContainerUrl() const override;
		Azure::Storage::Blobs::BlobServiceClient GetServiceClient() const override;
		Azure::Storage::Blobs::BlobContainerClient GetContainerClient() const override;
		Azure::Storage::Blobs::BlobClient GetBlobClient() const override;
		vector<string> UrlPathParts() const override;
	};
}
