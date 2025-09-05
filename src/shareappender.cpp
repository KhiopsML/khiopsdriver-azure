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
		nCurrentPos = client.GetProperties().Value.FileSize;
	}

	void ShareAppender::Close()
	{
		client.ForceCloseAllHandles();
	}

	size_t ShareAppender::Write(const void* source, size_t nSize, size_t nCount)
	{
		Azure::Storage::Files::Shares::Models::FileHttpHeaders httpHeaders;
		Azure::Storage::Files::Shares::Models::FileSmbProperties smbProperties;
		Azure::Storage::Files::Shares::SetFilePropertiesOptions opts;

		size_t nToWrite = nSize * nCount;

		Azure::Core::IO::MemoryBodyStream bodyStream((const uint8_t*)source, nToWrite);
		opts.Size = nCurrentPos + nToWrite;
		client.SetProperties(httpHeaders, smbProperties, opts);
		client.UploadRange(nCurrentPos, bodyStream);
		nCurrentPos += nToWrite;

		return nToWrite;
	}
}
