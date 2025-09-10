#include "filereader.hpp"

using namespace std;

namespace az
{
	FileReader::FileReader(vector<Azure::Storage::Blobs::BlobClient>&& clients) :
		FileReader(vector<ObjectClient>(clients.begin(), clients.end()))
	{}
	
	FileReader::FileReader(vector<Azure::Storage::Files::Shares::ShareFileClient>&& clients) :
		FileReader(vector<ObjectClient>(clients.begin(), clients.end()))
	{
	}

	FileReader::FileReader(vector<ObjectClient>&& clients) :
		FileStream(),
		storageType(clients.front().tag),
		fileInfo(clients),
		nCurrentPos(0)
	{
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
			unique_ptr<Azure::Core::IO::BodyStream>& bodyStream = fileInfo.GetBodyStreams().at(nFilePartIndex);
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

		size_t nDest = (size_t)nSignedDest;
		for (unique_ptr<Azure::Core::IO::BodyStream>& bodyStream : fileInfo.GetBodyStreams())
		{
			bodyStream->Rewind();
		}
		nCurrentPos = 0;
		constexpr size_t nBufferSize = 1024 * 1024; // TODO
		uint8_t* buffer = new uint8_t[nBufferSize];
		try
		{
			while (nCurrentPos < nDest)
			{
				Read(buffer, 1, nDest - nCurrentPos > nBufferSize ? nBufferSize : nDest - nCurrentPos);
			}
		}
		catch (...)
		{
			delete[] buffer;
			throw;
		}
		delete[] buffer;
	}
}
