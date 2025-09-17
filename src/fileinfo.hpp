#pragma once

namespace az
{
	class NoFilePartInfoError;
	class FileInfo;
	struct FilePartInfo;
	struct PartInfo;
}

#include <string>
#include <cstddef>
#include <vector>
#include <azure/storage/blobs/blob_client.hpp>
#include <azure/storage/files/shares/share_file_client.hpp>
#include <azure/core/io/body_stream.hpp>
#include "exception.hpp"
#include "storagetype.hpp"
#include "objectclient.hpp"

namespace az
{
	class NoFilePartInfoError : public Error
	{
	public: inline NoFilePartInfoError() : Error("no part info found in file info") {}
	};

	class FileInfo
	{
	public:
		FileInfo();
		FileInfo(std::vector<Azure::Storage::Blobs::BlobClient>&& clients);
		FileInfo(std::vector<Azure::Storage::Files::Shares::ShareFileClient>&& clients);
		FileInfo(std::vector<ObjectClient>&& clients);
		size_t GetSize() const;
		size_t GetHeaderLen() const;
		const PartInfo& GetPartInfo(size_t nIndex) const;
		size_t GetFilePartIndexOfUserOffset(size_t nUserOffset) const;
		
	private:
		StorageType storageType;
		size_t nHeaderLen;
		size_t nSize;
		std::vector<PartInfo> parts;
	};

	struct FilePartInfo
	{
		std::string sHeader;
		size_t nSize;
		ObjectClient client;

		FilePartInfo(std::string&& sHeader, size_t nSize, ObjectClient&& client);
		FilePartInfo(const FilePartInfo& other);
		FilePartInfo& operator=(FilePartInfo&& source);
	};

	struct PartInfo
	{
		size_t nUserOffset;
		size_t nContentSize;
		ObjectClient client;

		PartInfo(size_t nUserOffset, size_t nContentSize, ObjectClient&& client);
	};
}
