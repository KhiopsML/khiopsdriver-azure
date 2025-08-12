#include "blobfileinfo.hpp"
#include <string>
#include <algorithm>
#include <iterator>
#include <functional>
#include <azure/storage/blobs/blob_options.hpp>
#include <azure/core/http/http.hpp>
#include <azure/storage/blobs/rest_client.hpp>

using namespace std;
using BlobClient = Azure::Storage::Blobs::BlobClient;
using BodyStream = Azure::Core::IO::BodyStream;
using DownloadBlobOptions = Azure::Storage::Blobs::DownloadBlobOptions;
using HttpRange = Azure::Core::Http::HttpRange;
using DownloadBlobResult = Azure::Storage::Blobs::Models::DownloadBlobResult;

namespace az
{
	static vector<FilePartInfo> GetBlobFilePartInfo(const vector<BlobClient>& clients);
	static unique_ptr<BodyStream> GetBlobFilePartBodyStream(const BlobClient& client, size_t nOffset);
	static string ReadBlobHeaderFromBodyStream(unique_ptr<BodyStream>& bodyStream);

	BlobFileInfo::BlobFileInfo(const vector<BlobClient>& clients):
		FileInfo(GetBlobFilePartInfo(clients)),
		bodyStreams()
	{
		const size_t nHeaderLen = GetHeader().size();

		for (size_t i = 0; i < clients.size(); i++)
		{
			if (parts.at(i).nContentSize != 0)
			{
				bodyStreams.push_back(GetBlobFilePartBodyStream(clients.at(i), i == 0 ? 0 : nHeaderLen));
			}
		}

		remove_if(parts.begin(), parts.end(), [](const auto& part) { return part.nContentSize == 0; });
	}

	vector<unique_ptr<BodyStream>>& BlobFileInfo::GetBodyStreams()
	{
		return bodyStreams;
	}

	static vector<FilePartInfo> GetBlobFilePartInfo(const vector<BlobClient>& clients)
	{
		vector<FilePartInfo> result;
		for (const auto& client : clients)
		{
			DownloadBlobResult downloadBlobResult = move(client.Download().Value);
			string sHeader = ReadBlobHeaderFromBodyStream(downloadBlobResult.BodyStream);
			result.push_back(FilePartInfo{ sHeader, (size_t)downloadBlobResult.BlobSize });
		}
		return result;
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

	static unique_ptr<BodyStream> GetBlobFilePartBodyStream(const BlobClient& client, size_t nOffset)
	{
		DownloadBlobOptions opts;
		HttpRange range;
		range.Offset = nOffset;
		opts.Range = range;
		DownloadBlobResult downloadBlobResult = move(client.Download(opts).Value);
		return move(downloadBlobResult.BodyStream);
	}
}
