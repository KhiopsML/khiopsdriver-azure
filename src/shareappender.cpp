#include "shareappender.hpp"
#include <algorithm>
#include <iterator>
#include <sstream>
#include <iomanip>
#include <azure/core/base64.hpp>
#include <azure/storage/common/storage_exception.hpp>

using namespace std;

namespace az
{
	ShareAppender::ShareAppender(Azure::Storage::Files::Shares::ShareFileClient&& client) :
		client(client)
	{
		this->client.Create(0);
		nCurrentPos = client.GetProperties().Value.FileSize;
	}

	void ShareAppender::Close()
	{
		this->client.ForceCloseAllHandles();
	}

	size_t ShareAppender::Write(const void* source, size_t nSize, size_t nCount)
	{
		size_t nToWrite = nSize * nCount;
		size_t nWritten = 0;
		while (nToWrite > 0)
		{
			size_t n = nToWrite > 100 * 1024 * 1024 ? 100 * 1024 * 1024 : nToWrite;
			Azure::Core::IO::MemoryBodyStream bodyStream(((const uint8_t*)source) + nWritten, n);
			nToWrite -= n;
			nWritten += n;
			client.UploadRange(nCurrentPos, bodyStream);
			nCurrentPos += nWritten;
		}
		return nWritten;
	}
}
