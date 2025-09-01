#include "sharefileinfo.hpp"
#include <string>
#include <algorithm>
#include <iterator>
#include <functional>
#include <azure/storage/files/shares/share_options.hpp>
#include <azure/core/http/http.hpp>
#include <azure/storage/files/shares/share_responses.hpp>

using namespace std;
using ShareFileClient = Azure::Storage::Files::Shares::ShareFileClient;
using BodyStream = Azure::Core::IO::BodyStream;
using DownloadFileOptions = Azure::Storage::Files::Shares::DownloadFileOptions;
using HttpRange = Azure::Core::Http::HttpRange;
using DownloadFileResult = Azure::Storage::Files::Shares::Models::DownloadFileResult;

namespace az
{
	static vector<FilePartInfo> GetShareFilePartInfo(const vector<ShareFileClient>& clients);
	static unique_ptr<BodyStream> GetShareFilePartBodyStream(const ShareFileClient& client, size_t nOffset);
	static string ReadFileHeaderFromBodyStream(unique_ptr<BodyStream>& bodyStream);

	ShareFileInfo::ShareFileInfo(const vector<ShareFileClient>& clients) :
		FileInfo(GetShareFilePartInfo(clients)),
		bodyStreams()
	{
		const size_t nHeaderLen = GetHeader().size();

		for (size_t i = 0; i < clients.size(); i++)
		{
			if (parts.at(i).nContentSize != 0)
			{
				bodyStreams.push_back(GetShareFilePartBodyStream(clients.at(i), i == 0 ? 0 : nHeaderLen));
			}
		}

		remove_if(parts.begin(), parts.end(), [](const auto& part) { return part.nContentSize == 0; });
	}

	vector<unique_ptr<BodyStream>>& ShareFileInfo::GetBodyStreams()
	{
		return bodyStreams;
	}

	static vector<FilePartInfo> GetShareFilePartInfo(const vector<ShareFileClient>& clients)
	{
		vector<FilePartInfo> result;
		for (const auto& client : clients)
		{
			DownloadFileResult downloadFileResult = move(client.Download().Value);
			string sHeader = ReadFileHeaderFromBodyStream(downloadFileResult.BodyStream);
			result.push_back(FilePartInfo{ sHeader, (size_t)downloadFileResult.FileSize });
		}
		return result;
	}

	static string ReadFileHeaderFromBodyStream(unique_ptr<BodyStream>& bodyStream)
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

	static unique_ptr<BodyStream> GetShareFilePartBodyStream(const ShareFileClient& client, size_t nOffset)
	{
		DownloadFileOptions opts;
		HttpRange range;
		range.Offset = nOffset;
		opts.Range = range;
		DownloadFileResult downloadFileResult = move(client.Download(opts).Value);
		return move(downloadFileResult.BodyStream);
	}
}
