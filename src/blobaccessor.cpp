#include "blobaccessor.hpp"
#include <algorithm>
#include <iterator>
#include <functional>
#include <azure/core/http/transport.hpp>
#include "exception.hpp"
#include "util/connstring.hpp"
#include "util/string.hpp"
#include "util/env.hpp"
#include "util/glob.hpp"
#include "contrib/globmatch.hpp"

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

	vector<Azure::Storage::Blobs::Models::BlobItem> BlobAccessor::ListBlobs() const
	{
		vector<Azure::Storage::Blobs::Models::BlobItem> resultBlobs;
		auto resultBackInserter = back_inserter(resultBlobs);
		const string sContainerName = GetContainerName();
		const string sObjectName = GetObjectName();
		size_t nGlobbingCharPos = globbing::FindGlobbingChar(sObjectName);
		function<bool(const Azure::Storage::Blobs::Models::BlobItem&)> blobMatchesUrl;
		Azure::Storage::Blobs::ListBlobContainersOptions listBlobContainersOptions;
		listBlobContainersOptions.Prefix = sContainerName;
		Azure::Storage::Blobs::ListBlobsOptions listBlobsOptions;
		if (nGlobbingCharPos == string::npos)
		{
			// TODO: Is it relevant? Wouldn't globbing non-glob URLs lead to the same results? What would be the performance cost?
			blobMatchesUrl = [sObjectName](const Azure::Storage::Blobs::Models::BlobItem& blob)
			{
				return !blob.IsDeleted && blob.Name == sObjectName;
			};
			listBlobsOptions.Prefix = sObjectName;
		}
		else
		{
			blobMatchesUrl = [sObjectName](const Azure::Storage::Blobs::Models::BlobItem& blob)
			{
				return !blob.IsDeleted && globbing::GitignoreGlobMatch(blob.Name, sObjectName);
			};
			listBlobsOptions.Prefix = sObjectName.substr(0, nGlobbingCharPos);
		}
		try
		{
			for (auto pagedContainerList = GetServiceClient().ListBlobContainers(listBlobContainersOptions); pagedContainerList.HasPage(); pagedContainerList.MoveToNextPage())
			{
				auto foundContainerIt = find_if(pagedContainerList.BlobContainers.begin(), pagedContainerList.BlobContainers.end(), [sContainerName](const auto& container)
					{
						return !container.IsDeleted && container.Name == sContainerName;
					}
				);
				if (foundContainerIt != pagedContainerList.BlobContainers.end())
				{
					for (auto pagedBlobList = GetContainerClient().ListBlobs(listBlobsOptions); pagedBlobList.HasPage(); pagedBlobList.MoveToNextPage())
					{
						copy_if(pagedBlobList.Blobs.begin(), pagedBlobList.Blobs.end(), resultBackInserter, blobMatchesUrl);
					}
				}
			}
			return resultBlobs;
		}
		catch (const Azure::Core::Http::TransportException& exc)
		{
			throw NetworkError();
		}
	}
}
