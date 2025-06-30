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
			catch (const exception&)
			{
				return false;
			}
		}
	}

	//size_t BlobAccessor::GetSize() const
	//{
	//	// TODO: Implement
	//	return 0;
	//}

	//FileStream BlobAccessor::Open(char mode) const
	//{
	//	// TODO: Implement
	//	return FileStream();
	//}
}
