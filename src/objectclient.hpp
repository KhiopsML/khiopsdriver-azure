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
		ObjectClient(const ObjectClient& source) :
			tag(source.tag)
		{
			if (tag == StorageType::BLOB)
				new(&blob) Azure::Storage::Blobs::BlobClient(source.blob);
			else
				new(&shareFile) Azure::Storage::Files::Shares::ShareFileClient(source.shareFile);
		}
		ObjectClient(const Azure::Storage::Blobs::BlobClient& client) : tag(StorageType::BLOB), blob(client) {}
		ObjectClient(const Azure::Storage::Files::Shares::ShareFileClient& client) : tag(StorageType::SHARE), shareFile(client) {}
		~ObjectClient() {}
	};
}
