#pragma once

#include <vector>
#include <azure/storage/blobs/blob_container_client.hpp>
#include <azure/storage/blobs/blob_client.hpp>

namespace az
{
	std::vector<Azure::Storage::Blobs::BlobClient> ResolveBlobsSearchString(const Azure::Storage::Blobs::BlobContainerClient& containerClient, const std::string& sSearchString);
}
