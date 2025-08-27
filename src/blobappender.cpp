#include "blobappender.hpp"
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
	BlobAppender::BlobAppender(Azure::Storage::Blobs::BlobClient&& client):
		client(client.AsBlockBlobClient())
	{
	}

	void BlobAppender::Close()
	{
	}

	size_t BlobAppender::Write(const void* source, size_t nSize, size_t nCount)
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
		vector<string> blockIds;
		transform(blocks.begin(), blocks.end(), back_inserter(blockIds), [](const auto& block) { return block.Name; });
		size_t nToWrite = nSize * nCount;
		size_t nWritten = 0;
		while (nToWrite > 0)
		{
			string sBlockIdInBase10 = (ostringstream() << setfill('0') << setw(64) << blockIds.size()).str();
			vector<uint8_t> blockIdInBase10(sBlockIdInBase10.begin(), sBlockIdInBase10.end());
			string sBlockIdInBase64 = Azure::Core::Convert::Base64Encode(blockIdInBase10);
			size_t n = nToWrite > 100 * 1024 * 1024 ? 100 * 1024 * 1024 : nToWrite;
			Azure::Core::IO::MemoryBodyStream bodyStream(((const uint8_t*)source) + nWritten, n);
			nToWrite -= n;
			nWritten += n;
			client.StageBlock(sBlockIdInBase64, bodyStream);
			blockIds.push_back(sBlockIdInBase64);
		}
		client.CommitBlockList(blockIds);
		return nWritten;
	}
}
