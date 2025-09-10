#include "fileinfo.hpp"
#include <algorithm>
#include <numeric>
#include <azure/core/http/http.hpp>
#include <azure/storage/blobs/rest_client.hpp>
#include <azure/storage/files/shares/share_responses.hpp>
#include "util/random.hpp"

using namespace std;
using HttpRange = Azure::Core::Http::HttpRange;
using BodyStream = Azure::Core::IO::BodyStream;
using DownloadBlobOptions = Azure::Storage::Blobs::DownloadBlobOptions;
using DownloadFileOptions = Azure::Storage::Files::Shares::DownloadFileOptions;

namespace az
{
	static string ReadHeaderFromBodyStream(std::unique_ptr<BodyStream>&& bodyStream);
	static string GetFileHeader(std::vector<const FilePartInfo&>&& filePartInfos);
	static vector<PartInfo> GetFileParts(const vector<FilePartInfo>& filePartInfo, size_t nHeaderLen);

	static constexpr size_t nMaxHeaderSize = 8ULL * 1024 * 1024;

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
		HttpRange range { 0, nMaxHeaderSize };
		size_t nClients = clients.size();
		size_t nRandomlyPicked = 0;
		string sFilePartHeader;
		size_t nFilePartSize;
		vector<const FilePartInfo&> filePartsWithHeaderToInspect;
		for (size_t i = 0; i < nClients; i++)
		{
			sFilePartHeader = "";
			bool bGetHeader = false;
			if (i < 5 || nClients <= 10 || i >= nClients - 5)
			{
				bGetHeader = true;
			}
			else if (nRandomlyPicked < 10 && (i >= nClients - 15 + nRandomlyPicked || RandomBool()))
			{
				bGetHeader = true;
				nRandomlyPicked++;
			}
			if (clients[i].tag == StorageType::BLOB)
			{
				if (bGetHeader)
				{
					DownloadBlobOptions opts;
					opts.Range = move(range);
					auto downloadResult = move(clients[i].blob.Download(opts).Value);
					sFilePartHeader = ReadHeaderFromBodyStream(move(downloadResult.BodyStream));
					nFilePartSize = downloadResult.BlobSize;
				}
				else
				{
					nFilePartSize = (size_t)clients[i].blob.GetProperties().Value.BlobSize;
				}
			}
			else // SHARE storage
			{
				if (bGetHeader)
				{
					DownloadFileOptions opts;
					opts.Range = move(range);
					auto downloadResult = move(clients[i].shareFile.Download(opts).Value);
					sFilePartHeader = ReadHeaderFromBodyStream(move(downloadResult.BodyStream));
					nFilePartSize = downloadResult.FileSize;
				}
				else
				{
					nFilePartSize = (size_t)clients[i].shareFile.GetProperties().Value.FileSize;
				}
			}
			filePartInfos.emplace_back(move(sFilePartHeader), nFilePartSize, move(clients[i]));
			if (bGetHeader)
			{
				filePartsWithHeaderToInspect.push_back(filePartInfos.at(i));
			}
		}

		nHeaderLen = GetFileHeader(move(filePartsWithHeaderToInspect)).size();
		parts = GetFileParts(filePartInfos, nHeaderLen);
		nSize = accumulate(parts.begin(), parts.end(), 0ULL, [](size_t nTotal, const PartInfo& partInfo) { return nTotal + partInfo.nContentSize; });
	}

	size_t FileInfo::GetSize() const
	{
		return nSize;
	}

	size_t FileInfo::GetHeaderLen() const
	{
		return nHeaderLen;
	}

	const PartInfo& FileInfo::GetPartInfo(size_t nIndex) const
	{
		return parts.at(nIndex);
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
		size_t nBytesRead;
		uint8_t* bufferReadEnd;
		uint8_t* foundLineFeed;
		bool bFoundLineFeed;

		uint8_t* buffer = new uint8_t[nBufferSize];
		try
		{
			do
			{
				nBytesRead = bodyStream->ReadToCount(buffer, nBufferSize);
				bufferReadEnd = buffer + nBytesRead;
				bFoundLineFeed = (foundLineFeed = find(buffer, bufferReadEnd, '\n')) < bufferReadEnd;
				sHeader.append((const char*)buffer, bFoundLineFeed ? foundLineFeed + 1 - buffer : nBytesRead);
			} while (!bFoundLineFeed && nBytesRead == nBufferSize);
		}
		catch (...)
		{
			delete[] buffer;
			throw;
		}
		delete[] buffer;

		return bFoundLineFeed ? sHeader : "";
	}

	static string GetFileHeader(vector<const FilePartInfo&>&& filePartInfos)
	{
		string sFirstHeader = filePartInfos.front().sHeader;
		return sFirstHeader.empty()
			|| any_of(filePartInfos.begin() + 1, filePartInfos.end(), [sFirstHeader](const auto& partInfo) { partInfo.sHeader != sFirstHeader; })
			? string()
			: sFirstHeader;
	}

	static vector<PartInfo> GetFileParts(const vector<FilePartInfo>& filePartInfo, size_t nHeaderLen)
	{
		vector<PartInfo> result;
		size_t nUserOffset = 0;
		size_t nContentSize;
		bool bFirstIter = true;
		for (const auto& partInfo : filePartInfo)
		{
			nContentSize = bFirstIter ? partInfo.nSize : partInfo.nSize - nHeaderLen;
			if (nContentSize != 0)
			{
				result.emplace_back(nUserOffset, nContentSize, move(partInfo.client));
			}
			nUserOffset += nContentSize;
			bFirstIter = false;
		}
		return result;
	}
}
