#include "emulatedblobaccessor.hpp"
#include "exception.hpp"
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

	Azure::Storage::Blobs::BlobClient EmulatedBlobAccessor::GetBlobClient() const
	{
		const ConnectionString& connStr = GetConnectionString();
		return Azure::Storage::Blobs::BlobClient(
			GetUrl().GetAbsoluteUrl(),
			make_shared<Azure::Storage::StorageSharedKeyCredential>(connStr.sAccountName, connStr.sAccountKey)
		);
	}
}
