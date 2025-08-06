#include "blobreader.hpp"
#include <cstdio>
#include <memory>
#include <azure/core/io/body_stream.hpp>
#include "blobfileinfo.hpp"
#include "exception.hpp"

using namespace std;

namespace az
{
	BlobReader::BlobReader(vector<Azure::Storage::Blobs::BlobClient>&& clients) :
		FileReader(),
		clients(clients),
		fileInfo(BlobFileInfo(clients))
	{
	}

	void BlobReader::Close()
	{

	}

	size_t BlobReader::Read(void* dest, size_t size, size_t count)
	{
		size_t nTotalFileSize = fileInfo.GetSize();
		size_t nToRead = size * count;
		size_t nRead = 0;
		size_t nFilePartIndex;

		while (nToRead > 0 && nRead < nTotalFileSize)
		{
			nFilePartIndex = fileInfo.GetFilePartIndexOfUserOffset((long long int)nCurrentPos);
			unique_ptr<Azure::Core::IO::BodyStream>& bodyStream = fileInfo.GetBodyStreams().at(nFilePartIndex);
			dest = ((uint8_t*)dest) + nRead;
			nToRead -= (nRead = bodyStream->ReadToCount((uint8_t*)dest, nToRead));
			nCurrentPos += nRead;
		}
		return nRead;
	}

	void BlobReader::Seek(long long int offset, int whence)
	{
		switch (whence)
		{
		case SEEK_SET:
		case SEEK_CUR:
		case SEEK_END:
		default:
			throw InvalidSeekOriginError(whence);
		}
		size_t nFilePartIndex = fileInfo.GetFilePartIndexOfUserOffset((long long int)offset);
	}
}
