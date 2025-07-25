#include "blobaccessor.hpp"
#include <azure/core/http/transport.hpp>
#include "util/connstring.hpp"
#include "util/string.hpp"
#include "util/env.hpp"
#include "blobpathresolve.hpp"

namespace az
{
	BlobAccessor::~BlobAccessor()
	{
	}

	bool BlobAccessor::Exists() const
	{
		if (HasDirUrl())
		{
			CheckDirUrl();
			return true; // there is no such concept as a directory when dealing with blob services
		}
		else
		{
			CheckFileUrl();
			return !ListBlobs().empty();
		}
	}

	size_t BlobAccessor::GetSize() const
	{
		return 0; // TODO: Implement.
	}
	//size_t BlobAccessor::GetSize() const
	//{
	//	if (HasDirUrl())
	//	{
	//		// TODO: What should it do?
	//	}
	//	else
	//	{
	//		vector<Azure::Storage::Blobs::Models::BlobItem> blobs = ListBlobs();
	//		return accumulate(blobs.begin(), blobs.end(), 0,
	//			[](size_t acc, const Azure::Storage::Blobs::Models::BlobItem& elem)
	//			{
	//				return acc + elem.BlobSize;
	//			}
	//		);
	//	}
	//}

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

	vector<Azure::Storage::Blobs::BlobClient> BlobAccessor::ListBlobs() const
	{
		try
		{
			return ResolveBlobsSearchString(GetContainerClient(), GetObjectName());
		}
		catch (const Azure::Core::Http::TransportException& exc)
		{
			throw NetworkError();
		}
	}
}
