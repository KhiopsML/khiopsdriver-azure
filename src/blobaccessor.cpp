#include "blobaccessor.hpp"
#include <numeric>
#include <algorithm>
#include <azure/core/http/transport.hpp>
#include "util/connstring.hpp"
#include "util/string.hpp"
#include "util/env.hpp"
#include "blobpathresolve.hpp"

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
			return accumulate(blobs.begin(), blobs.end(), 0,
				[](size_t total, const Azure::Storage::Blobs::BlobClient& blob)
				{
					return total + blob.GetProperties().Value.BlobSize;
				}
			);
		}
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

	string BlobAccessor::ReadHeader(const Azure::Storage::Blobs::BlobClient& blobClient) const
	{
		string sHeader = "";
		constexpr size_t nBufferSize = 4096;
		uint8_t buffer[nBufferSize];
		size_t nBytesRead;
		uint8_t* bufferReadEnd;
		uint8_t* foundLineFeed;
		bool bFoundLineFeed;
		auto bodyStream = blobClient.Download().Value.BodyStream;
		do
		{
			nBytesRead = bodyStream->ReadToCount(buffer, nBufferSize);
			bufferReadEnd = buffer + nBytesRead;
			bFoundLineFeed = (foundLineFeed = find(buffer, bufferReadEnd, '\n')) < bufferReadEnd;
			sHeader.append((const char*)buffer, bFoundLineFeed ? foundLineFeed - buffer : nBytesRead);
		} while (!bFoundLineFeed && nBytesRead == nBufferSize);
		return sHeader;
	}
}
