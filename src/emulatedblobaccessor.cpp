#include "emulatedblobaccessor.hpp"

namespace az
{
	EmulatedBlobAccessor::EmulatedBlobAccessor(const Azure::Core::Url& url, const string& sConnectionString):
		BlobAccessor(url),
		EmulatedFileAccessor(sConnectionString)
	{
	}

	EmulatedBlobAccessor::~EmulatedBlobAccessor()
	{
	}

	Azure::Storage::Blobs::BlobClient EmulatedBlobAccessor::GetBlobClient() const
	{
		return Azure::Storage::Blobs::BlobClient(GetUrl().GetAbsoluteUrl(), make_shared<Azure::Storage::StorageSharedKeyCredential>(sAccountName, sAccountKey));
	}
}
