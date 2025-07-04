#include "blobaccessor.hpp"
#include <algorithm>
#include <azure/core/http/transport.hpp>
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
			return true; // there is no such concept as a directory when dealing with blob services
		}
		else
		{
			try
			{
				const vector<string>& pathParts = UrlPathParts();
				auto containers = GetServiceClient().ListBlobContainers().BlobContainers;
				auto foundContainerIt = find_if(containers.begin(), containers.end(), [&pathParts](const auto& container) { return container.Name == pathParts.at(0); });
				if (foundContainerIt == containers.end())
				{
					return false;
				}
				auto blobs = GetContainerClient().ListBlobs().Blobs;
				auto foundBlobIt = find_if(blobs.begin(), blobs.end(), [&pathParts](const auto& blob) { return blob.Name == pathParts.at(1); });
				return foundBlobIt != blobs.end();
			}
			catch (const Azure::Core::Http::TransportException& exc)
			{
				throw NetworkError();
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
