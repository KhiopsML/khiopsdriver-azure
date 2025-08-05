#include "blobreader.hpp"
#include <algorithm>
#include <iterator>

using namespace std;

namespace az
{
	BlobReader::BlobReader(const std::vector<Azure::Storage::Blobs::BlobClient>& clients) :
		FileReader()
	{
		size_t nOffset = 0;
		for (const auto& client : clients)
		{
			const auto downloadBlobResult = client.Download().Value;
			bodyStreamsByOffset.emplace(nOffset, downloadBlobResult.BodyStream);
		}
	}

	void BlobReader::Close()
	{

	}

	size_t BlobReader::Read(void* dest, size_t size, size_t count)
	{

	}

	void BlobReader::Seek(long long int offset, int whence)
	{

	}
}
