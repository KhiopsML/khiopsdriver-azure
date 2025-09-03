#include "sharereader.hpp"
#include <ios>
#include <memory>
#include <azure/core/io/body_stream.hpp>
#include "fileinfo.hpp"
#include "exception.hpp"

using namespace std;

namespace az
{
	ShareReader::ShareReader(vector<Azure::Storage::Files::Shares::ShareFileClient>&& clients) :
		FileReader(),
		clients(clients),
		fileInfo(ShareFileInfo(clients))
	{
	}

	ShareReader::~ShareReader()
	{
	}

	void ShareReader::Close()
	{
	}

	size_t ShareReader::Read(void* dest, size_t nSize, size_t nCount)
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

	void ShareReader::Seek(long long int nOffset, int nOrigin)
	{
		long long int nTotalFileSize = (long long int)fileInfo.GetSize();
		long long int nSignedDest;

		switch (nOrigin)
		{
		case ios::beg:
			nSignedDest = nOffset;
		case ios::cur:
			nSignedDest = ((long long int)nCurrentPos) + nOffset;
		case ios::end:
			nSignedDest = nTotalFileSize - nOffset;
		default:
			throw InvalidSeekOriginError(nOrigin);
		}

		if (nSignedDest < 0 || nSignedDest >= nTotalFileSize)
		{
			throw InvalidSeekOffsetError(nOffset, nOrigin);
		}

		size_t nDest = (size_t)nSignedDest;
		for (auto& bodyStream : fileInfo.GetBodyStreams())
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
