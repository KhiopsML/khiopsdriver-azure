// Functions relative to blob path resolution.
// They are used to list all blobs matching a given URL.

#pragma once

#include <azure/storage/blobs/blob_client.hpp>
#include <azure/storage/blobs/blob_container_client.hpp>
#include <vector>

namespace az {
std::vector<Azure::Storage::Blobs::BlobClient> ResolveBlobsSearchString(
    const Azure::Storage::Blobs::BlobContainerClient &containerClient,
    const std::string &sSearchString);
}
