#include "filestream.hpp"
#include <chrono>
#include <iomanip>
#include <sstream>
#include <azure/storage/blobs/block_blob_client.hpp>
#include <azure/storage/common/storage_exception.hpp>

using namespace std;

namespace az
{
	FileStreamType operator&(const FileStreamType& a, const FileStreamType& b)
	{
		return (FileStreamType)((int)a & (int)b);
	}

	FileStream::~FileStream() {}

	void* FileStream::GetHandle() const
	{
		return handle;
	}

	FileStream::FileStream() :
		handle((void*)chrono::steady_clock::now().time_since_epoch().count())
	{
	}

	static unique_ptr<Azure::Core::IO::BodyStream> DownloadFilePart(const ObjectClient& client, size_t nOffset, size_t nLength);

	FileReader::FileReader(vector<Azure::Storage::Blobs::BlobClient>&& clients) :
		FileReader(vector<ObjectClient>(clients.begin(), clients.end()))
	{
	}

	FileReader::FileReader(vector<Azure::Storage::Files::Shares::ShareFileClient>&& clients) :
		FileReader(vector<ObjectClient>(clients.begin(), clients.end()))
	{
	}

	FileReader::FileReader(vector<ObjectClient>&& clients) :
		FileStream(),
		nCurrentPos(0)
	{
		if (clients.empty())
		{
			throw invalid_argument("cannot instantiate a file reader with no clients");
		}
		storageType = clients.front().tag;
		fileInfo = move(FileInfo(move(clients)));
	}

	FileReader::~FileReader()
	{
		Close();
	}

	void FileReader::Close()
	{
	}

	size_t FileReader::Read(void* dest, size_t nSize, size_t nCount)
	{
		size_t nTotalFileSize = fileInfo.GetSize();
		size_t nToRead = nSize * nCount;
		size_t nRead = 0;
		size_t nTotalRead = 0;
		size_t nFilePartIndex = fileInfo.GetFilePartIndexOfUserOffset(nCurrentPos);

		while (nToRead != 0 && nCurrentPos != nTotalFileSize)
		{
			const PartInfo& partInfo = fileInfo.GetPartInfo(nFilePartIndex);
			unique_ptr<Azure::Core::IO::BodyStream> bodyStream = move(
				DownloadFilePart(
					partInfo.client,
					(nFilePartIndex == 0 ? 0 : fileInfo.GetHeaderLen()) + nCurrentPos - partInfo.nUserOffset,
					nToRead < partInfo.nContentSize ? nToRead : partInfo.nContentSize
				)
			);
			nToRead -= (nRead = bodyStream->ReadToCount((uint8_t*)dest, nToRead));
			nTotalRead += nRead;
			nCurrentPos += nRead;
			dest = (uint8_t*)dest + nRead;
			nFilePartIndex++;
		}
		return nTotalRead;
	}

	void FileReader::Seek(long long int nOffset, int nOrigin)
	{
		size_t nTotalFileSize = fileInfo.GetSize();
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

	static unique_ptr<Azure::Core::IO::BodyStream> DownloadFilePart(const ObjectClient& client, size_t nOffset, size_t nLength)
	{
		Azure::Core::Http::HttpRange range{ (int64_t)nOffset, (int64_t)nLength };

		if (client.tag == BLOB)
		{
			Azure::Storage::Blobs::DownloadBlobOptions opts;
			opts.Range = range;
			auto downloadResult = move(client.blob.Download(opts).Value);
			return move(downloadResult.BodyStream);
		}
		else // SHARE storage
		{
			Azure::Storage::Files::Shares::DownloadFileOptions opts;
			opts.Range = range;
			auto downloadResult = move(client.shareFile.Download(opts).Value);
			return move(downloadResult.BodyStream);
		}
	}

	FileWriter::FileWriter(FileOutputMode mode, Azure::Storage::Blobs::BlobClient&& client) :
		FileWriter(mode, move(ObjectClient(client)))
	{
	}

	FileWriter::FileWriter(FileOutputMode mode, Azure::Storage::Files::Shares::ShareFileClient&& client) :
		FileWriter(mode, move(ObjectClient(client)))
	{
	}

	FileWriter::FileWriter(FileOutputMode mode, ObjectClient&& client) :
		FileStream(),
		storageType(client.tag),
		mode(mode),
		client(client),
		nCurrentPos(0),
		blockIds()
	{
		if (storageType == BLOB)
		{
			if (mode == FileOutputMode::APPEND)
			{
				try
				{
					vector<Azure::Storage::Blobs::Models::BlobBlock> blocks;
					auto blockListRequestResponse = this->client.blob.AsBlockBlobClient().GetBlockList();
					blocks = blockListRequestResponse.Value.CommittedBlocks;
					transform(blocks.begin(), blocks.end(), back_inserter(blockIds), [](const auto& block) { return block.Name; });
				}
				catch (const Azure::Storage::StorageException&)
				{
				}
			}
		}
		else // SHARE storage
		{
			if (mode == FileOutputMode::WRITE)
				this->client.shareFile.Create(0);
			else  // APPEND mode
				nCurrentPos = (size_t)this->client.shareFile.GetProperties().Value.FileSize;
		}
	}

	FileWriter::~FileWriter()
	{
		Close();
	}

	void FileWriter::Close()
	{
		Flush();
	}

	size_t FileWriter::Write(const void* source, size_t nSize, size_t nCount)
	{

		if (storageType == BLOB)
		{
			Azure::Storage::Blobs::BlockBlobClient bbclient = client.blob.AsBlockBlobClient();

			size_t nToWrite = nSize * nCount;

			string sBlockIdInBase10 = (ostringstream() << setfill('0') << setw(64) << blockIds.size()).str();
			vector<uint8_t> blockIdInBase10(sBlockIdInBase10.begin(), sBlockIdInBase10.end());
			string sBlockIdInBase64 = Azure::Core::Convert::Base64Encode(blockIdInBase10);

			Azure::Core::IO::MemoryBodyStream bodyStream((const uint8_t*)source, nToWrite);
			bbclient.StageBlock(sBlockIdInBase64, bodyStream);
			blockIds.push_back(sBlockIdInBase64);

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
			client.shareFile.SetProperties(httpHeaders, smbProperties, opts);
			client.shareFile.UploadRange((int64_t)nCurrentPos, bodyStream);
			nCurrentPos += nToWrite;

			return nToWrite;
		}
	}

	void FileWriter::Flush()
	{
		if (storageType == BLOB)
		{
			client.blob.AsBlockBlobClient().CommitBlockList(blockIds);
		}
		else
		{
			client.shareFile.ForceCloseAllHandles();
		}
	}
}
