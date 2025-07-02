#include "blobaccessor.hpp"
#include "exception.hpp"
#include "util/connstring.hpp"
#include "util/string.hpp"
#include "util/env.hpp"

namespace az
{
	BlobAccessor::~BlobAccessor()
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
				GetBlobClient().GetProperties();
				return true;
			}
			catch (const exception& exc)
			{
				auto what = exc.what();//Azure::Core::Http::TransportException
				return false;
			}
		}
	}

	size_t BlobAccessor::GetSize() const
	{
		// TODO: Implement
		return 0;
	}

	FileStream BlobAccessor::Open(char mode) const
	{
		// TODO: Implement
		return FileStream();
	}

	void BlobAccessor::Remove() const
	{
		// TODO: Implement
	}

	void BlobAccessor::MkDir() const
	{
		// TODO: Implement
	}

	void BlobAccessor::RmDir() const
	{
		// TODO: Implement
	}

	size_t BlobAccessor::GetFreeDiskSpace() const
	{
		// TODO: Implement
		return 0;
	}

	void BlobAccessor::CopyTo(const Azure::Core::Url& destUrl) const
	{
		// TODO: Implement
	}

	void BlobAccessor::CopyFrom(const Azure::Core::Url& sourceUrl) const
	{
		// TODO: Implement
	}

	BlobAccessor::BlobAccessor(const Azure::Core::Url& url):
		FileAccessor(url)
	{
	}
}
