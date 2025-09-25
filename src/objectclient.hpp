// Tagged union abstracting the concepts of blob clients and share file clients to a single "object client".

#pragma once

#include <azure/storage/blobs/blob_client.hpp>
#include <azure/storage/files/shares/share_file_client.hpp>
#include "storagetype.hpp"

namespace az
{
	struct ObjectClient
	{
		StorageType tag;
		union
		{
			Azure::Storage::Blobs::BlobClient blob;
			Azure::Storage::Files::Shares::ShareFileClient shareFile;
		};
		ObjectClient(const ObjectClient& source);
		ObjectClient(const Azure::Storage::Blobs::BlobClient& client);
		ObjectClient(const Azure::Storage::Files::Shares::ShareFileClient& client);
		~ObjectClient();
		ObjectClient& operator=(const ObjectClient& source);
	};
}
