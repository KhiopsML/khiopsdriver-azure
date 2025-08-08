#include "blobfileinfo.hpp"
#include <string>
#include <algorithm>
#include <iterator>
#include <functional>
#include <azure/storage/blobs/blob_options.hpp>
#include <azure/core/http/http.hpp>
#include <azure/storage/blobs/rest_client.hpp>
#include "util/map.hpp"

using namespace std;
using BlobClient = Azure::Storage::Blobs::BlobClient;
using BodyStream = Azure::Core::IO::BodyStream;
using DownloadBlobOptions = Azure::Storage::Blobs::DownloadBlobOptions;
using HttpRange = Azure::Core::Http::HttpRange;
using DownloadBlobResult = Azure::Storage::Blobs::Models::DownloadBlobResult;

namespace az
{
	static FilePartInfo GetBlobFilePartInfo(const BlobClient& blobClient);
	static unique_ptr<BodyStream> GetBlobFilePartBodyStream(const BlobClient& client, size_t nOffset);
	static string ReadBlobHeaderFromBodyStream(unique_ptr<BodyStream>& bodyStream);

	BlobFileInfo::BlobFileInfo(const vector<BlobClient>& clients):
		FileInfo(MAP(BlobClient, FilePartInfo, clients, GetBlobFilePartInfo)),
		bodyStreams()
	{
		bodyStreams.push_back(GetBlobFilePartBodyStream(clients.front(), 0));
		transform(clients.begin() + 1, clients.end(), back_inserter(bodyStreams), bind(GetBlobFilePartBodyStream, placeholders::_1, GetHeader().length()));
	}

	vector<unique_ptr<BodyStream>>& BlobFileInfo::GetBodyStreams()
	{
		return bodyStreams;
	}

	static FilePartInfo GetBlobFilePartInfo(const BlobClient& client)
	{
		DownloadBlobResult downloadBlobResult = move(client.Download().Value);
		return FilePartInfo { ReadBlobHeaderFromBodyStream(downloadBlobResult.BodyStream), (size_t)downloadBlobResult.BlobSize };
	}

	static unique_ptr<BodyStream> GetBlobFilePartBodyStream(const BlobClient& client, size_t nOffset)
	{
		DownloadBlobOptions opts;
		HttpRange range;
		range.Offset = nOffset;
		opts.Range = range;
		DownloadBlobResult downloadBlobResult = move(client.Download(opts).Value);
		return move(downloadBlobResult.BodyStream);
	}

	static string ReadBlobHeaderFromBodyStream(unique_ptr<BodyStream>& bodyStream)
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
			nBytesRead = bodyStream->ReadToCount(buffer, nBufferSize);
			bufferReadEnd = buffer + nBytesRead;
			bFoundLineFeed = (foundLineFeed = find(buffer, bufferReadEnd, '\n')) < bufferReadEnd;
			sHeader.append((const char*)buffer, bFoundLineFeed ? foundLineFeed + 1 - buffer : nBytesRead);
		} while (!bFoundLineFeed && nBytesRead == nBufferSize);

		return bFoundLineFeed ? sHeader : "";
	}
}
