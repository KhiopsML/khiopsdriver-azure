#pragma once

namespace az
{
	class BlobReader;
}

#include <cstddef>
#include <vector>
#include <map>
#include <utility>
#include <memory>
#include <azure/storage/blobs/blob_client.hpp>
#include <azure/core/io/body_stream.hpp>
#include "filereader.hpp"

namespace az
{
	class BlobReader : public FileReader
	{
	public:
		BlobReader(const std::vector<Azure::Storage::Blobs::BlobClient>& clients);

		void Close() override;
		size_t Read(void* dest, size_t size, size_t count) override;
		void Seek(long long int offset, int whence) override;

	private:
		std::map<size_t, std::unique_ptr<Azure::Core::IO::BodyStream>> bodyStreamsByOffset;
	};
}
