#pragma once

namespace az
{
	class BlobWriter;
}

#include <cstddef>
#include <memory>
#include <azure/storage/blobs/blob_client.hpp>
#include <azure/storage/blobs/block_blob_client.hpp>
#include <azure/core/io/body_stream.hpp>
#include "filewriter.hpp"

namespace az
{
	class BlobWriter : public FileWriter
	{
	public:
		BlobWriter(Azure::Storage::Blobs::BlobClient&& client);

		size_t Write(const void* source, size_t size, size_t count) override;

	private:
		Azure::Storage::Blobs::BlockBlobClient client;
		std::unique_ptr<Azure::Core::IO::BodyStream> bodyStream;
	};
}
