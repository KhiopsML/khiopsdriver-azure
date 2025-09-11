#include "fileoutputstream.hpp"
#include <memory>
#include <iomanip>
#include <azure/storage/blobs/block_blob_client.hpp>
#include <azure/core/base64.hpp>
#include <azure/storage/common/storage_exception.hpp>

using namespace std;

namespace az
{
	FileOutputStream::FileOutputStream(FileOutputMode mode, Azure::Storage::Blobs::BlobClient&& client):
		FileOutputStream(mode, move(ObjectClient(client)))
	{
	}

	FileOutputStream::FileOutputStream(FileOutputMode mode, Azure::Storage::Files::Shares::ShareFileClient&& client):
		FileOutputStream(mode, move(ObjectClient(client)))
	{
	}

	FileOutputStream::FileOutputStream(FileOutputMode mode, ObjectClient&& client):
		FileStream(),
		storageType(client.tag),
		mode(mode),
		client(move(client)),
		nCurrentPos(0)
	{
		if (storageType == StorageType::BLOB)
		{
			if (mode == FileOutputMode::WRITE)
				this->client.blob.AsBlockBlobClient().CommitBlockList({});
		}
		else // SHARE storage
		{
			if (mode == FileOutputMode::WRITE)
				this->client.shareFile.Create(0);
			else  // APPEND mode
				nCurrentPos = this->client.shareFile.GetProperties().Value.FileSize;
		}
	}

	void FileOutputStream::Close()
	{
		if (storageType == StorageType::SHARE)
			client.shareFile.ForceCloseAllHandles();
	}

	size_t FileOutputStream::Write(const void* source, size_t nSize, size_t nCount)
	{
		
		if (storageType == StorageType::BLOB)
		{
			Azure::Storage::Blobs::BlockBlobClient bbclient = client.blob.AsBlockBlobClient();
			vector<Azure::Storage::Blobs::Models::BlobBlock> blocks;
			try
			{
				auto blockListRequestResponse = bbclient.GetBlockList();
				blocks = blockListRequestResponse.Value.CommittedBlocks;
			}
			catch (const Azure::Storage::StorageException& exc)
			{
			}

			size_t nToWrite = nSize * nCount;

			vector<string> blockIds;
			transform(blocks.begin(), blocks.end(), back_inserter(blockIds), [](const auto& block) { return block.Name; });
			string sBlockIdInBase10 = (ostringstream() << setfill('0') << setw(64) << blockIds.size()).str();
			vector<uint8_t> blockIdInBase10(sBlockIdInBase10.begin(), sBlockIdInBase10.end());
			string sBlockIdInBase64 = Azure::Core::Convert::Base64Encode(blockIdInBase10);

			Azure::Core::IO::MemoryBodyStream bodyStream((const uint8_t*)source, nToWrite);
			bbclient.StageBlock(sBlockIdInBase64, bodyStream);
			blockIds.push_back(sBlockIdInBase64);
			bbclient.CommitBlockList(blockIds);

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
			client.shareFile.UploadRange(nCurrentPos, bodyStream);
			nCurrentPos += nToWrite;

			return nToWrite;
		}
	}

	void FileOutputStream::Flush()
	{
		// Do nothing
	}
}
