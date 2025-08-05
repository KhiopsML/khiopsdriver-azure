#pragma once

#include "filepartinfo.hpp"
#include <azure/storage/blobs/blob_client.hpp>

namespace az
{
	FilePartInfo GetBlobFilePartInfo(const Azure::Storage::Blobs::BlobClient& blobClient);
}
