#include "cloudblobaccessor.hpp"

namespace az
{
	CloudBlobAccessor::CloudBlobAccessor(const Azure::Core::Url& url):
		BlobAccessor(url),
		CloudFileAccessor()
	{
	}

	CloudBlobAccessor::~CloudBlobAccessor()
	{
	}

	Azure::Storage::Blobs::BlobClient CloudBlobAccessor::GetBlobClient() const
	{
		return Azure::Storage::Blobs::BlobClient(GetUrl().GetAbsoluteUrl(), GetCredential());
	}
}
