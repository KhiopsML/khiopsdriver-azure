#include "blobaccessor.hpp"
#include <memory>
#include <numeric>
#include <algorithm>
#include <iterator>
#include <fstream>
#include <azure/core/http/transport.hpp>
#include "util/connstring.hpp"
#include "util/string.hpp"
#include "util/env.hpp"
#include "blobpathresolve.hpp"
#include "fileinfo.hpp"

using namespace std;
using BlobClient = Azure::Storage::Blobs::BlobClient;
using DeleteBlobOptions = Azure::Storage::Blobs::DeleteBlobOptions;
using DeleteSnapshotsOption = Azure::Storage::Blobs::Models::DeleteSnapshotsOption;
using Url = Azure::Core::Url;
using TransportException = Azure::Core::Http::TransportException;

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
			throw InvalidOperationForDirError(Operation::GET_SIZE);
		}
		else
		{
			auto blobs = ListBlobs();
			if (blobs.empty())
			{
				throw NoFileError(GetUrl().GetAbsoluteUrl());
			}
			return FileInfo(blobs).GetSize();
		}
	}

	FileReader& BlobAccessor::OpenForReading() const
	{
		return RegisterReader(move(FileReader(move(ListBlobs()))));
	}

	FileOutputStream& BlobAccessor::OpenForWriting() const
	{
		return RegisterWriter(move(FileOutputStream(FileOutputMode::WRITE, move(GetBlobClient()))));
	}

	FileOutputStream& BlobAccessor::OpenForAppending() const
	{
		return RegisterWriter(move(FileOutputStream(FileOutputMode::APPEND, move(GetBlobClient()))));
	}

	void BlobAccessor::Remove() const
	{
		auto blobs = ListBlobs();
		if (blobs.empty())
		{
			throw NoFileError(GetUrl().GetAbsoluteUrl());
		}
		DeleteBlobOptions opts;
		opts.DeleteSnapshots = DeleteSnapshotsOption::IncludeSnapshots;
		for (const auto& blob : blobs)
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
		auto& reader = OpenForReading();
		constexpr size_t nBufferSize = 4ULL * 1024 * 1024; // TODO
		char* buffer = new char[nBufferSize];
		size_t nRead;
		ofstream ofs(destUrl, ios::binary);

		while ((nRead = reader.Read(buffer, 1, nBufferSize)) > 0)
		{
			ofs.write(buffer, nRead);
		}

		delete[] buffer;
	}

	void BlobAccessor::CopyFrom(const string& sourceUrl) const
	{
		auto& writer = OpenForWriting();
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
			writer.Write(buffer, 1, nRead);
		}

		delete[] buffer;
	}

	BlobAccessor::BlobAccessor(const Azure::Core::Url& url, const function<FileReader& (FileReader&&)>& registerReader, const function<FileOutputStream& (FileOutputStream&&)>& registerWriter) :
		FileAccessor(url, registerReader, registerWriter)
	{
	}

	vector<Azure::Storage::Blobs::BlobClient> BlobAccessor::ListBlobs() const
	{
		try
		{
			return ResolveBlobsSearchString(GetContainerClient(), GetObjectName());
		}
		catch (const TransportException& exc)
		{
			throw NetworkError();
		}
	}
}
