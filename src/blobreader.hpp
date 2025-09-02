#pragma once

namespace az
{
	class BlobReader;
}

#include <cstddef>
#include <vector>
#include <azure/storage/blobs/blob_client.hpp>
#include "filereader.hpp"
#include "fileinfo.hpp"

namespace az
{
	class BlobReader : public FileReader
	{
	public:
		BlobReader(std::vector<Azure::Storage::Blobs::BlobClient>&& clients);
		~BlobReader();

		void Close() override;
		size_t Read(void* dest, size_t nSize, size_t nCount) override;
		void Seek(long long int nOffset, int nOrigin) override;

	private:
		std::vector<Azure::Storage::Blobs::BlobClient> clients;
		BlobFileInfo fileInfo;
	};
}
