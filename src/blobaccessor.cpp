#include "blobaccessor.hpp"
#include <memory>
#include <numeric>
#include <algorithm>
#include <fstream>
#include <azure/core/http/transport.hpp>
#include "util/connstring.hpp"
#include "util/string.hpp"
#include "util/env.hpp"
#include "blobpathresolve.hpp"
#include "fileinfo.hpp"
#include "blobreader.hpp"
#include "blobwriter.hpp"
#include "blobappender.hpp"

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

	const unique_ptr<FileReader>& BlobAccessor::OpenForReading() const
	{
		vector<Azure::Storage::Blobs::BlobClient> blobs = ListBlobs();
		return RegisterReader(make_unique<BlobReader>(move(blobs)));
	}

	const unique_ptr<FileOutputStream>& BlobAccessor::OpenForWriting() const
	{
		return (const unique_ptr<FileOutputStream>&)RegisterWriter(make_unique<BlobWriter>(move(GetBlobClient())));
	}

	const unique_ptr<FileOutputStream>& BlobAccessor::OpenForAppending() const
	{
		return (const unique_ptr<FileOutputStream>&)RegisterAppender(make_unique<BlobAppender>(move(GetBlobClient())));
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

	void BlobAccessor::CopyTo(const string& destUrl) const
	{
		const auto& reader = OpenForReading();
		constexpr size_t nBufferSize = 4ULL * 1024 * 1024; // TODO
		char* buffer = new char[nBufferSize];
		size_t nRead;
		ofstream ofs(destUrl, ios::binary);

		while ((nRead = reader->Read(buffer, 1, nBufferSize)) > 0)
		{
			ofs.write(buffer, nRead);
		}

		delete[] buffer;
	}

	void BlobAccessor::CopyFrom(const string& sourceUrl) const
	{
		const auto& writer = OpenForWriting();
		constexpr size_t nBufferSize = 4ULL * 1024 * 1024; // TODO
		char* buffer = new char[nBufferSize];
		size_t nRead;
		ifstream ifs(sourceUrl, ios::binary);

		for (;;)
		{
			ifs.read(buffer, nBufferSize);
			nRead = (size_t)ifs.gcount();
			if (nRead == 0)
			{
				break;
			}
			writer->Write(buffer, 1, nRead);
		}

		delete[] buffer;
	}

	BlobAccessor::BlobAccessor(const Azure::Core::Url& url, const function<const unique_ptr<FileReader>& (unique_ptr<FileReader>)>& registerReader, const function<const unique_ptr<FileWriter>& (unique_ptr<FileWriter>)>& registerWriter, const function<const unique_ptr<FileAppender>& (unique_ptr<FileAppender>)>& registerAppender) :
		FileAccessor(url, registerReader, registerWriter, registerAppender)
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
