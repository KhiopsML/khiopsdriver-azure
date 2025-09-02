#include "fileinfo.hpp"
#include <algorithm>
#include <numeric>
#include <azure/core/http/http.hpp>
#include <azure/storage/blobs/rest_client.hpp>
#include <azure/storage/files/shares/share_responses.hpp>

using namespace std;
using BlobClient = Azure::Storage::Blobs::BlobClient;
using DownloadBlobOptions = Azure::Storage::Blobs::DownloadBlobOptions;
using ShareFileClient = Azure::Storage::Files::Shares::ShareFileClient;
using DownloadFileOptions = Azure::Storage::Files::Shares::DownloadFileOptions;
using HttpRange = Azure::Core::Http::HttpRange;
using BodyStream = Azure::Core::IO::BodyStream;

namespace az
{
	const char* NoFilePartInfoError::what() const noexcept
	{
		return "no part info found in file info";
	}

	size_t FileInfoBase::GetSize() const
	{
		return nSize;
	}

	size_t FileInfoBase::GetFilePartIndexOfUserOffset(size_t nUserOffset) const
	{
		if (parts.empty())
		{
			throw NoFilePartInfoError();
		}
		return (size_t)(find_if(parts.begin(), parts.end(), [nUserOffset](const auto& part) { return nUserOffset < part.nUserOffset; }) - 1 - parts.begin());
	}

	std::vector<std::unique_ptr<BodyStream>>& FileInfoBase::GetBodyStreams()
	{
		return bodyStreams;
	}

	FileInfoBase::FileInfoBase() :
		sHeader(),
		nSize(0),
		parts(),
		bodyStreams()
	{
	}

	string FileInfoBase::ReadHeaderFromBodyStream(unique_ptr<BodyStream>& bodyStream)
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

	string FileInfoBase::GetFileHeader(const vector<FilePartInfo>& filePartInfo)
	{
		string sFirstHeader = filePartInfo.front().sHeader;
		return sFirstHeader.empty()
			|| any_of(filePartInfo.begin() + 1, filePartInfo.end(), [sFirstHeader](const auto& partInfo) { return partInfo.sHeader != sFirstHeader; })
			? string()
			: sFirstHeader;
	}

	vector<FileInfoBase::PartInfo> FileInfoBase::GetFileParts(const vector<FilePartInfo>& filePartInfo, size_t nHeaderLen)
	{
		vector<PartInfo> parts;
		size_t nRealOffset = 0;
		size_t nUserOffset = 0;
		size_t nContentSize;
		bool bFirstIter = true;
		for (const auto& partInfo : filePartInfo)
		{
			nContentSize = bFirstIter ? partInfo.nSize : partInfo.nSize - nHeaderLen;
			parts.push_back(PartInfo{ nRealOffset, nUserOffset, nContentSize });
			if (bFirstIter)
			{
				nRealOffset += nHeaderLen;
			}
			nRealOffset += partInfo.nSize;
			nUserOffset += nContentSize;
			bFirstIter = false;
		}
		return parts;
	}

	BlobFileInfo::BlobFileInfo() :
		FileInfoBase()
	{
	}

	BlobFileInfo::BlobFileInfo(const vector<BlobClient>& clients):
		FileInfoBase()
	{
		vector<FilePartInfo> fileParts;
		for (const auto& client : clients)
		{
			auto downloadResult = move(client.Download().Value);
			string sHeader_ = ReadHeaderFromBodyStream(downloadResult.BodyStream);
			fileParts.push_back(FilePartInfo{ sHeader_, (size_t)downloadResult.BlobSize });
		}

		sHeader = GetFileHeader(fileParts);
		parts = GetFileParts(fileParts, sHeader.length());
		nSize = accumulate(parts.begin(), parts.end(), 0ULL, [](size_t nTotal, const PartInfo& partInfo) { return nTotal + partInfo.nContentSize; });

		const size_t nHeaderLen = sHeader.size();

		for (size_t i = 0; i < clients.size(); i++)
		{
			if (parts.at(i).nContentSize != 0)
			{
				bodyStreams.push_back(DownloadFilePart(clients.at(i), i == 0 ? 0 : nHeaderLen));
			}
		}

		auto it = remove_if(parts.begin(), parts.end(), [](const auto& part) { return part.nContentSize == 0; });
		parts.erase(it, parts.end());
	}

	unique_ptr<BodyStream> BlobFileInfo::DownloadFilePart(const BlobClient& client, size_t nOffset)
	{
		DownloadBlobOptions opts;
		HttpRange range;
		range.Offset = (int64_t)nOffset;
		opts.Range = range;
		auto downloadResult = move(client.Download(opts).Value);
		return move(downloadResult.BodyStream);
	}

	ShareFileInfo::ShareFileInfo() :
		FileInfoBase()
	{
	}

	ShareFileInfo::ShareFileInfo(const vector<ShareFileClient>& clients) :
		FileInfoBase()
	{
		vector<FilePartInfo> fileParts;
		for (const auto& client : clients)
		{
			auto downloadResult = move(client.Download().Value);
			string sHeader_ = ReadHeaderFromBodyStream(downloadResult.BodyStream);
			fileParts.push_back(FilePartInfo{ sHeader_, (size_t)downloadResult.FileSize });
		}

		sHeader = GetFileHeader(fileParts);
		parts = GetFileParts(fileParts, sHeader.length());
		nSize = accumulate(parts.begin(), parts.end(), 0ULL, [](size_t nTotal, const PartInfo& partInfo) { return nTotal + partInfo.nContentSize; });

		const size_t nHeaderLen = sHeader.size();

		for (size_t i = 0; i < clients.size(); i++)
		{
			if (parts.at(i).nContentSize != 0)
			{
				bodyStreams.push_back(DownloadFilePart(clients.at(i), i == 0 ? 0 : nHeaderLen));
			}
		}

		auto it = remove_if(parts.begin(), parts.end(), [](const auto& part) { return part.nContentSize == 0; });
		parts.erase(it, parts.end());
	}

	unique_ptr<BodyStream> ShareFileInfo::DownloadFilePart(const ShareFileClient& client, size_t nOffset)
	{
		DownloadFileOptions opts;
		HttpRange range;
		range.Offset = (int64_t)nOffset;
		opts.Range = range;
		auto downloadResult = move(client.Download(opts).Value);
		return move(downloadResult.BodyStream);
	}
}
