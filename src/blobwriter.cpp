#include "blobwriter.hpp"
#include <algorithm>
#include <iterator>
#include <sstream>
#include <iomanip>
#include <azure/core/http/http_status_code.hpp>
#include <azure/storage/blobs/rest_client.hpp>
#include <azure/core/base64.hpp>
#include <azure/storage/common/storage_exception.hpp>

using namespace std;

namespace az
{
	BlobWriter::BlobWriter(Azure::Storage::Blobs::BlobClient&& client):
		client(client.AsBlockBlobClient())
	{
		this->client.CommitBlockList({});
	}

	void BlobWriter::Close()
	{
	}

	size_t BlobWriter::Write(const void* source, size_t nSize, size_t nCount)
	{
		// TODO: Optimize this function
		vector<Azure::Storage::Blobs::Models::BlobBlock> blocks;
		try
		{
			auto blockListRequestResponse = client.GetBlockList();
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
		client.StageBlock(sBlockIdInBase64, bodyStream);
		blockIds.push_back(sBlockIdInBase64);
		client.CommitBlockList(blockIds);

		return nToWrite;
	}
}
