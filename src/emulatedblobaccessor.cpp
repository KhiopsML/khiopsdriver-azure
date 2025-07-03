#include "emulatedblobaccessor.hpp"
#include "util/connstring.hpp"

namespace az
{
	EmulatedBlobAccessor::EmulatedBlobAccessor(const Azure::Core::Url& url):
		BlobAccessor(url),
		EmulatedFileAccessor()
	{
		if (!IsConnectionStringCompatibleWithUrl(url))
		{
			throw IncompatibleConnectionStringError();
		}
	}

	EmulatedBlobAccessor::~EmulatedBlobAccessor()
	{
	}

	template<typename ClientT> ClientT EmulatedBlobAccessor::GetClient() const
	{
		const ConnectionString& connStr = GetConnectionString();
		return ClientT(GetUrl().GetAbsoluteUrl(), GetCredential());
	}
	
	Azure::Storage::Blobs::BlobServiceClient EmulatedBlobAccessor::GetServiceClient() const
	{
		return GetClient<Azure::Storage::Blobs::BlobServiceClient>();
	}

	Azure::Storage::Blobs::BlobContainerClient EmulatedBlobAccessor::GetContainerClient() const
	{
		return GetClient<Azure::Storage::Blobs::BlobContainerClient>();
	}

	Azure::Storage::Blobs::BlobClient EmulatedBlobAccessor::GetBlobClient() const
	{
		return GetClient<Azure::Storage::Blobs::BlobClient>();
	}
}
