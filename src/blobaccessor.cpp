#include "blobaccessor.h"
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
	}

	size_t BlobAccessor::GetSize() const
	{

	}

	FileStream BlobAccessor::Open() const
	{

	}
}
