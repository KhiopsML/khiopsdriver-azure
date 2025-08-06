#include "blobfileinfo.hpp"
#include <string>
#include <memory>

using namespace std;

namespace az
{
	FilePartInfo GetBlobFilePartInfo(const Azure::Storage::Blobs::BlobClient& blobClient)
	{
		const auto downloadBlobResult = move(blobClient.Download().Value);
		return FilePartInfo{ ReadBlobHeaderFromBodyStream(*downloadBlobResult.BodyStream), (size_t)downloadBlobResult.BlobSize };
	}

	static string ReadBlobHeaderFromBodyStream(Azure::Core::IO::BodyStream& bodyStream)
	{
		string sHeader = "";
		constexpr size_t nBufferSize = 4096;
		uint8_t buffer[nBufferSize];
		size_t nBytesRead;
		uint8_t* bufferReadEnd;
		uint8_t* foundLineFeed;
		bool bFoundLineFeed;

		do
		{
			nBytesRead = bodyStream.ReadToCount(buffer, nBufferSize);
			bufferReadEnd = buffer + nBytesRead;
			bFoundLineFeed = (foundLineFeed = find(buffer, bufferReadEnd, '\n')) < bufferReadEnd;
			sHeader.append((const char*)buffer, bFoundLineFeed ? foundLineFeed + 1 - buffer : nBytesRead);
		} while (!bFoundLineFeed && nBytesRead == nBufferSize);

		return bFoundLineFeed ? sHeader : "";
	}
}
