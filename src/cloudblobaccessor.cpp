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

	template<typename ClientT> ClientT CloudBlobAccessor::GetClient() const
	{
		const ConnectionString& connStr = GetConnectionString();
		return ClientT(GetUrl().GetAbsoluteUrl(), GetCredential());
	}

	Azure::Storage::Blobs::BlobServiceClient CloudBlobAccessor::GetServiceClient() const
	{
		return GetClient<Azure::Storage::Blobs::BlobServiceClient>();
	}

	Azure::Storage::Blobs::BlobContainerClient CloudBlobAccessor::GetContainerClient() const
	{
		return GetClient<Azure::Storage::Blobs::BlobContainerClient>();
	}

	Azure::Storage::Blobs::BlobClient CloudBlobAccessor::GetBlobClient() const
	{
		return GetClient<Azure::Storage::Blobs::BlobClient>();
	}
}
