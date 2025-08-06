#include "blobaccessor.hpp"
#include <memory>
#include <numeric>
#include <algorithm>
#include <azure/core/http/transport.hpp>
#include "util/connstring.hpp"
#include "util/string.hpp"
#include "util/env.hpp"
#include "blobpathresolve.hpp"
#include "blobreader.hpp"

using namespace std;

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
			return !ListBlobs().empty();
		}
	}

	size_t BlobAccessor::GetSize() const
	{
		if (HasDirUrl())
		{
			throw GettingSizeOfDirError();
		}
		else
		{
			vector<Azure::Storage::Blobs::BlobClient> blobs = ListBlobs();
			if (blobs.empty())
			{
				throw NoFileError(GetUrl().GetAbsoluteUrl());
			}
			return BlobFileInfo(blobs).GetSize();
		}
	}

	unique_ptr<FileReader> BlobAccessor::OpenForReading() const
	{
		vector<Azure::Storage::Blobs::BlobClient> blobs = ListBlobs();
		return make_unique<BlobReader>(move(blobs));
	}

	void BlobAccessor::Remove() const
	{
		vector<Azure::Storage::Blobs::BlobClient> blobs = ListBlobs();
		if (blobs.empty())
		{
			throw NoFileError(GetUrl().GetAbsoluteUrl());
		}
		Azure::Storage::Blobs::DeleteBlobOptions opts;
		opts.DeleteSnapshots = Azure::Storage::Blobs::Models::DeleteSnapshotsOption::IncludeSnapshots;
		for (const Azure::Storage::Blobs::BlobClient& blob : blobs)
		{
			const string sBlobUrl = blob.GetUrl();
			if (!blob.Delete(opts).Value.Deleted)
			{
				throw DeletionError(sBlobUrl);
			}
		}
	}

	void BlobAccessor::MkDir() const
	{
		// Do nothing
	}

	void BlobAccessor::RmDir() const
	{
		// Do nothing
	}

	size_t BlobAccessor::GetFreeDiskSpace() const
	{
		return 5LL * 1024LL * 1024LL * 1024LL * 1024LL;
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
