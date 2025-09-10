#pragma once

namespace az
{
	class FileReader;
}

#include <cstddef>
#include <vector>
#include <azure/storage/blobs/blob_client.hpp>
#include <azure/storage/files/shares/share_file_client.hpp>
#include "filestream.hpp"
#include "storagetype.hpp"
#include "objectclient.hpp"
#include "fileinfo.hpp"

namespace az
{
	class FileReader : public FileStream
	{
	public:
		FileReader(std::vector<Azure::Storage::Blobs::BlobClient>&& clients);
		FileReader(std::vector<Azure::Storage::Files::Shares::ShareFileClient>&& clients);
		FileReader(std::vector<ObjectClient>&& clients);
		void Close() override;
		size_t Read(void* dest, size_t nSize, size_t nCount);
		void Seek(long long int nOffset, int nOrigin);

	private:
		StorageType storageType;
		FileInfo fileInfo;
		size_t nCurrentPos;
	};
}
