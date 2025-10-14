#include "filestream.hpp"
#include <chrono>
#include <iomanip>
#include <sstream>
#include <azure/storage/blobs/block_blob_client.hpp>
#include <azure/storage/common/storage_exception.hpp>

using namespace std;

namespace az
{
	FileStream FileStream::OpenForReading(const std::vector<Azure::Storage::Blobs::BlobClient>& clients)
	{
		return OpenForReading(vector<ObjectClient>(clients.begin(), clients.end()));
	}

	FileStream FileStream::OpenForReading(const std::vector<Azure::Storage::Files::Shares::ShareFileClient>& clients)
	{
		return OpenForReading(vector<ObjectClient>(clients.begin(), clients.end()));
	}

	FileStream FileStream::OpenForReading(const std::vector<ObjectClient>& clients)
	{
		if (clients.empty()) throw invalid_argument("cannot open a file for reading with no storage clients");
		FileStream fs;
		fs.storageType = clients.front().tag;
		fs.mode = Mode::READ;
		new(&fs.readInfo) FragmentedFile(clients);
		return fs;
	}

	FileStream FileStream::OpenForWriting(OutputMode mode, const Azure::Storage::Blobs::BlobClient& client)
	{
		return OpenForWriting(mode, ObjectClient(client));
	}

	FileStream FileStream::OpenForWriting(OutputMode mode, const Azure::Storage::Files::Shares::ShareFileClient& client)
	{
		return OpenForWriting(mode, ObjectClient(client));
	}

	FileStream FileStream::OpenForWriting(OutputMode mode, const ObjectClient& client)
	{
		FileStream fs;
		fs.storageType = client.tag;
		fs.mode = Mode::WRITE;
		new(&fs.writeInfo) WriteInfo(mode, client, vector<string>());

		if (fs.storageType == BLOB)
		{
			if (fs.writeInfo.mode == OutputMode::APPEND)
			{
				try
				{
					vector<Azure::Storage::Blobs::Models::BlobBlock> blocks;
					auto blockListRequestResponse = fs.writeInfo.client.blob.AsBlockBlobClient().GetBlockList();
					blocks = blockListRequestResponse.Value.CommittedBlocks;
					transform(blocks.begin(), blocks.end(), back_inserter(fs.writeInfo.blockIds), [](const auto& block) { return block.Name; });
				}
				catch (const Azure::Storage::StorageException&)
				{
				}
			}
		}
		else // SHARE storage
		{
			if (fs.writeInfo.mode == OutputMode::WRITE)
				fs.writeInfo.client.shareFile.Create(0);
			else  // APPEND mode
				fs.nCurrentPos = (size_t)fs.writeInfo.client.shareFile.GetProperties().Value.FileSize;
		}
		return fs;
	}

	FileStream::FileStream(FileStream&& source) :
		handle(std::move(source.handle)),
		storageType(std::move(source.storageType)),
		mode(std::move(source.mode)),
		nCurrentPos(std::move(source.nCurrentPos))
	{
		if (mode == Mode::READ) new(&readInfo) FragmentedFile(std::move(source.readInfo));
		else new(&writeInfo) WriteInfo(std::move(source.writeInfo));
	}

	FileStream::~FileStream()
	{
		if (mode == Mode::READ) readInfo.~FragmentedFile();
		else writeInfo.~WriteInfo();
	}

	void* FileStream::GetHandle() const
	{
		return handle;
	}

	FileStream::FileStream() :
		handle((void*)chrono::steady_clock::now().time_since_epoch().count()),
		nCurrentPos(0ULL)
	{
	}

	FileStream::WriteInfo::WriteInfo(OutputMode mode, const ObjectClient& client, const std::vector<std::string>& blockIds):
		mode(mode),
		client(client),
		blockIds(blockIds)
	{}

	FileStream::WriteInfo::WriteInfo(WriteInfo&& source):
		mode(std::move(source.mode)),
		client(source.client),
		blockIds(std::move(source.blockIds))
	{}

	FileStream::WriteInfo::~WriteInfo()
	{
		blockIds.clear();
	}

	void FileStream::Close()
	{
		if (mode == Mode::WRITE) Flush();
	}

	size_t FileStream::Read(void* dest, size_t nSize, size_t nCount)
	{
		if (mode != Mode::READ) throw InvalidOperationForStreamModeError("read", mode);

		size_t nTotalFileSize = readInfo.GetSize();
		size_t nToRead = nSize * nCount;
		size_t nRead = 0;
		size_t nTotalRead = 0;
		size_t nFragmentIndex = readInfo.GetFragmentIndexOfUserOffset(nCurrentPos);

		while (nToRead != 0)
		{
			const FragmentedFile::Fragment& fragment = readInfo.GetFragment(nFragmentIndex);

			Azure::Core::Http::HttpRange range {
				(int64_t)((nFragmentIndex == 0 ? 0 : readInfo.GetHeaderLen()) + nCurrentPos - fragment.nUserOffset),
				(int64_t)(nToRead < fragment.nContentSize ? nToRead : fragment.nContentSize)
			};

			unique_ptr<Azure::Core::IO::BodyStream> bodyStream;

			try
			{
				if (fragment.client.tag == BLOB)
				{
					Azure::Storage::Blobs::BlobAccessConditions accessConditions;
					accessConditions.IfMatch = fragment.etag;
					Azure::Storage::Blobs::DownloadBlobOptions opts;
					opts.AccessConditions = accessConditions;
					opts.Range = range;
					auto downloadResult = std::move(fragment.client.blob.Download(opts).Value);
					bodyStream = std::move(downloadResult.BodyStream);
				}
				else // SHARE storage
				{
					Azure::Storage::Files::Shares::DownloadFileOptions opts;
					opts.Range = range;
					auto downloadResult = std::move(fragment.client.shareFile.Download(opts).Value);
					if (downloadResult.Details.ETag != fragment.etag)
						throw ReadingUpdatedFileError();
					bodyStream = std::move(downloadResult.BodyStream);
				}
			}
			catch (const Azure::Storage::StorageException& exc)
			{
				if (exc.StatusCode == Azure::Core::Http::HttpStatusCode::RangeNotSatisfiable) throw ReadAtEOFError();
				if (exc.StatusCode == Azure::Core::Http::HttpStatusCode::PreconditionFailed) throw ReadingUpdatedFileError();
				throw;
			}

			nToRead -= (nRead = bodyStream->ReadToCount((uint8_t*)dest, nToRead));
			nTotalRead += nRead;
			nCurrentPos += nRead;
			dest = (uint8_t*)dest + nRead;
			if (nCurrentPos == nTotalFileSize) break;
			nFragmentIndex++;
		}
		return nTotalRead;
	}

	void FileStream::Seek(long long int nOffset, int nOrigin)
	{
		if (mode != Mode::READ) throw InvalidOperationForStreamModeError("seek", mode);

		size_t nTotalFileSize = readInfo.GetSize();
		long long int nSignedDest;

		switch (nOrigin)
		{
		case ios::beg:
			nSignedDest = nOffset;
			break;
		case ios::cur:
			nSignedDest = (long long int)nCurrentPos + nOffset;
			break;
		case ios::end:
			nSignedDest = (long long int)nTotalFileSize + nOffset;
			break;
		default:
			throw InvalidSeekOriginError(nOrigin);
		}

		if (nSignedDest < 0 || nSignedDest >= (long long int)nTotalFileSize)
		{
			throw InvalidSeekOffsetError(nOffset, nOrigin);
		}

		nCurrentPos = (size_t)nSignedDest;
	}

	size_t FileStream::Write(const void* source, size_t nSize, size_t nCount)
	{
		if (mode != Mode::WRITE) throw InvalidOperationForStreamModeError("write", mode);

		if (storageType == BLOB)
		{
			Azure::Storage::Blobs::BlockBlobClient bbclient = writeInfo.client.blob.AsBlockBlobClient();

			size_t nToWrite = nSize * nCount;

			string sBlockIdInBase10 = (ostringstream() << setfill('0') << setw(64) << writeInfo.blockIds.size()).str();
			vector<uint8_t> blockIdInBase10(sBlockIdInBase10.begin(), sBlockIdInBase10.end());
			string sBlockIdInBase64 = Azure::Core::Convert::Base64Encode(blockIdInBase10);

			Azure::Core::IO::MemoryBodyStream bodyStream((const uint8_t*)source, nToWrite);
			bbclient.StageBlock(sBlockIdInBase64, bodyStream);
			writeInfo.blockIds.push_back(sBlockIdInBase64);

			return nToWrite;
		}
		else // SHARE storage
		{
			Azure::Storage::Files::Shares::Models::FileHttpHeaders httpHeaders;
			Azure::Storage::Files::Shares::Models::FileSmbProperties smbProperties;
			Azure::Storage::Files::Shares::SetFilePropertiesOptions opts;

			size_t nToWrite = nSize * nCount;

			Azure::Core::IO::MemoryBodyStream bodyStream((const uint8_t*)source, nToWrite);
			opts.Size = nCurrentPos + nToWrite;
			writeInfo.client.shareFile.SetProperties(httpHeaders, smbProperties, opts);
			writeInfo.client.shareFile.UploadRange((int64_t)nCurrentPos, bodyStream);
			nCurrentPos += nToWrite;

			return nToWrite;
		}
	}

	void FileStream::Flush()
	{
		if (mode != Mode::WRITE) throw InvalidOperationForStreamModeError("flush", mode);

		if (storageType == BLOB)
		{
			writeInfo.client.blob.AsBlockBlobClient().CommitBlockList(writeInfo.blockIds);
		}
		else
		{
			writeInfo.client.shareFile.ForceCloseAllHandles();
		}
	}
}
