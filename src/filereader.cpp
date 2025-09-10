#include "filereader.hpp"

using namespace std;

namespace az
{
	static unique_ptr<Azure::Core::IO::BodyStream> DownloadFilePart(const ObjectClient& client, size_t nOffset, size_t nLength);

	FileReader::FileReader(vector<Azure::Storage::Blobs::BlobClient>&& clients) :
		FileReader(vector<ObjectClient>(clients.begin(), clients.end()))
	{}
	
	FileReader::FileReader(vector<Azure::Storage::Files::Shares::ShareFileClient>&& clients) :
		FileReader(vector<ObjectClient>(clients.begin(), clients.end()))
	{
	}

	FileReader::FileReader(vector<ObjectClient>&& clients) :
		FileStream(),
		nCurrentPos(0)
	{
		if (clients.empty())
		{
			throw invalid_argument("cannot instantiate a file reader with no clients");
		}
		storageType = clients.front().tag;
		fileInfo = move(FileInfo(move(clients)));
	}

	void FileReader::Close()
	{
	}

	size_t FileReader::Read(void* dest, size_t nSize, size_t nCount)
	{
		size_t nTotalFileSize = fileInfo.GetSize();
		size_t nToRead = nSize * nCount;
		size_t nRead = 0;
		size_t nTotalRead = 0;
		size_t nFilePartIndex = fileInfo.GetFilePartIndexOfUserOffset((long long int)nCurrentPos);

		while (nToRead > 0 && nCurrentPos < nTotalFileSize)
		{
			const PartInfo& partInfo = fileInfo.GetPartInfo(nFilePartIndex);
			unique_ptr<Azure::Core::IO::BodyStream> bodyStream = move(
				DownloadFilePart(
					partInfo.client,
					nFilePartIndex == 0 ? 0 : fileInfo.GetHeaderLen(),
					nToRead < partInfo.nContentSize ? nToRead : partInfo.nContentSize
				)
			);
			nToRead -= (nRead = bodyStream->ReadToCount((uint8_t*)dest, nToRead));
			nTotalRead += nRead;
			nCurrentPos += nRead;
			dest = ((uint8_t*)dest) + nRead;
			nFilePartIndex++;
		}
		return nTotalRead;
	}

	void FileReader::Seek(long long int nOffset, int nOrigin)
	{
		long long int nTotalFileSize = (long long int)fileInfo.GetSize();
		long long int nSignedDest;

		switch (nOrigin)
		{
		case ios::beg:
			nSignedDest = nOffset;
			break;
		case ios::cur:
			nSignedDest = ((long long int)nCurrentPos) + nOffset;
			break;
		case ios::end:
			nSignedDest = nTotalFileSize + nOffset;
			break;
		default:
			throw InvalidSeekOriginError(nOrigin);
		}

		if (nSignedDest < 0 || nSignedDest >= nTotalFileSize)
		{
			throw InvalidSeekOffsetError(nOffset, nOrigin);
		}

		nCurrentPos = (size_t)nSignedDest;
	}

	static unique_ptr<Azure::Core::IO::BodyStream> DownloadFilePart(const ObjectClient& client, size_t nOffset, size_t nLength)
	{
		Azure::Core::Http::HttpRange range{ (int64_t)nOffset, (int64_t)nLength };

		if (client.tag == StorageType::BLOB)
		{
			Azure::Storage::Blobs::DownloadBlobOptions opts;
			opts.Range = range;
			auto downloadResult = move(client.blob.Download(opts).Value);
			return move(downloadResult.BodyStream);
		}
		else // SHARE storage
		{
			Azure::Storage::Files::Shares::DownloadFileOptions opts;
			opts.Range = range;
			auto downloadResult = move(client.shareFile.Download(opts).Value);
			return move(downloadResult.BodyStream);
		}
	}
}
