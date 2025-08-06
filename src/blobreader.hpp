#pragma once

namespace az
{
	class BlobReader;
}

#include <cstddef>
#include <vector>
#include <azure/storage/blobs/blob_client.hpp>
#include "filereader.hpp"
#include "blobfileinfo.hpp"

namespace az
{
	class BlobReader : public FileReader
	{
	public:
		BlobReader(std::vector<Azure::Storage::Blobs::BlobClient>&& clients);

		void Close() override;
		size_t Read(void* dest, size_t size, size_t count) override;
		void Seek(long long int offset, int whence) override;

	private:
		std::vector<Azure::Storage::Blobs::BlobClient> clients;
		BlobFileInfo fileInfo;
	};
}
