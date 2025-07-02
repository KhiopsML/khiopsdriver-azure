#include "emulatedblobaccessor.hpp"

namespace az
{
	EmulatedBlobAccessor::EmulatedBlobAccessor(const Azure::Core::Url& url):
		BlobAccessor(url),
		EmulatedFileAccessor()
	{
	}

	EmulatedBlobAccessor::~EmulatedBlobAccessor()
	{
	}

	Azure::Storage::Blobs::BlobClient EmulatedBlobAccessor::GetBlobClient() const
	{
		return (Azure::Storage::Blobs::BlobClient)nullptr;
		//return Azure::Storage::Blobs::BlobClient(GetUrl().GetAbsoluteUrl(), make_shared<Azure::Storage::StorageSharedKeyCredential>(sAccountName, sAccountKey));
	}
}
