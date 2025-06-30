#include "blobaccessor.h"
#include <azure/storage/blobs.hpp>
#include "exception.h"

namespace az
{
	BlobAccessor::BlobAccessor(const Azure::Core::Url& url):
		FileAccessor(url)
	{
	}

	bool BlobAccessor::Exists() const
	{
		if (HasDirUrl())
		{
			return true;
		}
		else
		{
			try
			{
				Azure::Storage::Blobs::BlobClient(GetUrl().GetAbsoluteUrl(), GetCredential()).GetProperties();
				return true;
			}
			catch (const exception& exc)
			{
				return false;
			}
		}
	}

	size_t BlobAccessor::GetSize() const
	{

	}

	FileStream BlobAccessor::Open(char mode) const
	{

	}
}
