#pragma once

namespace az
{
	class BlobFileInfo;
}

#include <vector>
#include <memory>
#include <azure/storage/blobs/blob_client.hpp>
#include <azure/core/io/body_stream.hpp>
#include "fileinfo.hpp"

namespace az
{
	class BlobFileInfo : public FileInfo
	{
	public:
		BlobFileInfo(const std::vector<Azure::Storage::Blobs::BlobClient>& clients);
		std::vector<std::unique_ptr<Azure::Core::IO::BodyStream>>& GetBodyStreams();

	private:
		std::vector<std::unique_ptr<Azure::Core::IO::BodyStream>> bodyStreams;
	};
}
