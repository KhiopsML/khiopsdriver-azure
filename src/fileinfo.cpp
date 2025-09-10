#include "fileinfo.hpp"
#include <algorithm>
#include <numeric>
#include <azure/core/http/http.hpp>
#include <azure/storage/blobs/rest_client.hpp>
#include <azure/storage/files/shares/share_responses.hpp>

using namespace std;
using HttpRange = Azure::Core::Http::HttpRange;
using BodyStream = Azure::Core::IO::BodyStream;
using DownloadBlobOptions = Azure::Storage::Blobs::DownloadBlobOptions;
using DownloadFileOptions = Azure::Storage::Files::Shares::DownloadFileOptions;

namespace az
{
	static string ReadHeaderFromBodyStream(std::unique_ptr<BodyStream>&& bodyStream);
	static string GetFileHeader(const std::vector<FilePartInfo>& filePartInfo);
	static vector<PartInfo> GetFileParts(const vector<FilePartInfo>& filePartInfo, size_t nHeaderLen);
	static unique_ptr<Azure::Core::IO::BodyStream> DownloadFilePart(const ObjectClient& client, size_t nOffset);

	FileInfo::FileInfo() :
		nSize(0)
	{}

	FileInfo::FileInfo(const vector<Azure::Storage::Blobs::BlobClient>& clients) :
		FileInfo(vector<ObjectClient>(clients.begin(), clients.end()))
	{}

	FileInfo::FileInfo(const vector<Azure::Storage::Files::Shares::ShareFileClient>& clients) :
		FileInfo(vector<ObjectClient>(clients.begin(), clients.end()))
	{}

	FileInfo::FileInfo(const vector<ObjectClient>& clients):
		storageType(clients.front().tag)
	{
		if (clients.empty())
		{
			FileInfo();
			return;
		}

		vector<FilePartInfo> filePartInfos;
		for (const auto& client : clients)
		{
			if (client.tag == StorageType::BLOB)
			{
				auto downloadResult = move(client.blob.Download().Value);
				string sHeader_ = ReadHeaderFromBodyStream(move(downloadResult.BodyStream));
				filePartInfos.emplace_back(move(sHeader_), (size_t)downloadResult.BlobSize, move(client));
			}
			else // SHARE storage
			{
				auto downloadResult = move(client.shareFile.Download().Value);
				string sHeader_ = ReadHeaderFromBodyStream(move(downloadResult.BodyStream));
				filePartInfos.emplace_back(move(sHeader_), (size_t)downloadResult.FileSize, move(client));
			}
		}

		sHeader = GetFileHeader(filePartInfos);
		parts = GetFileParts(filePartInfos, sHeader.size());
		nSize = accumulate(parts.begin(), parts.end(), 0ULL, [](size_t nTotal, const PartInfo& partInfo) { return nTotal + partInfo.nContentSize; });
	}

	size_t FileInfo::GetSize() const
	{
		return nSize;
	}

	size_t FileInfo::GetFilePartIndexOfUserOffset(size_t nUserOffset) const
	{
		if (parts.empty())
		{
			throw NoFilePartInfoError();
		}
		return (size_t)(find_if(parts.begin(), parts.end(), [nUserOffset](const auto& part) { return nUserOffset < part.nUserOffset; }) - 1 - parts.begin());
	}

	static string ReadHeaderFromBodyStream(unique_ptr<BodyStream>&& bodyStream)
	{
		string sHeader;
		constexpr size_t nBufferSize = 4096; // TODO
		constexpr size_t nMaxHeaderSize = 8ULL * 1024 * 1024;
		size_t nBytesRead;
		size_t nTotalBytesRead = 0;
		uint8_t* bufferReadEnd;
		uint8_t* foundLineFeed;
		bool bFoundLineFeed;

		uint8_t* buffer = new uint8_t[nBufferSize];
		try
		{
			do
			{
				nTotalBytesRead += (nBytesRead = bodyStream->ReadToCount(buffer, nBufferSize));
				bufferReadEnd = buffer + nBytesRead;
				bFoundLineFeed = (foundLineFeed = find(buffer, bufferReadEnd, '\n')) < bufferReadEnd;
				sHeader.append((const char*)buffer, bFoundLineFeed ? foundLineFeed + 1 - buffer : nBytesRead);
			} while (!bFoundLineFeed && nBytesRead == nBufferSize && nTotalBytesRead < nMaxHeaderSize);
		}
		catch (...)
		{
			delete[] buffer;
			throw;
		}
		delete[] buffer;

		return bFoundLineFeed ? sHeader : "";
	}

	static string GetFileHeader(const vector<FilePartInfo>& filePartInfo)
	{
		string sFirstHeader = filePartInfo.front().sHeader;
		return sFirstHeader.empty()
			|| any_of(filePartInfo.begin() + 1, filePartInfo.end(), [sFirstHeader](const auto& partInfo) { return partInfo.sHeader != sFirstHeader; })
			? string()
			: sFirstHeader;
	}

	static vector<PartInfo> GetFileParts(const vector<FilePartInfo>& filePartInfo, size_t nHeaderLen)
	{
		vector<PartInfo> result;
		size_t nRealOffset = 0;
		size_t nUserOffset = 0;
		size_t nContentSize;
		bool bFirstIter = true;
		for (const auto& partInfo : filePartInfo)
		{
			nContentSize = bFirstIter ? partInfo.nSize : partInfo.nSize - nHeaderLen;
			if (nContentSize != 0)
			{
				result.emplace_back(nRealOffset, nUserOffset, nContentSize, move(partInfo.client));
			}
			if (bFirstIter)
			{
				nRealOffset += nHeaderLen;
			}
			nRealOffset += partInfo.nSize;
			nUserOffset += nContentSize;
			bFirstIter = false;
		}
		return result;
	}

	static unique_ptr<BodyStream> DownloadFilePart(const ObjectClient& client, size_t nOffset)
	{
		HttpRange range;
		range.Offset = (int64_t)nOffset;

		if (client.tag == StorageType::BLOB)
		{
			DownloadBlobOptions opts;
			opts.Range = range;
			auto downloadResult = move(client.blob.Download(opts).Value);
			return move(downloadResult.BodyStream);
		}
		else // SHARE storage
		{
			DownloadFileOptions opts;
			opts.Range = range;
			auto downloadResult = move(client.shareFile.Download(opts).Value);
			return move(downloadResult.BodyStream);
		}
	}
}
