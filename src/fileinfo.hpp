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
#include <memory>
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
	public:
		inline virtual const char* what() const noexcept override
		{
			return "no part info found in file info";
		}
	};

	class FileInfo
	{
	public:
		FileInfo();
		FileInfo(const std::vector<Azure::Storage::Blobs::BlobClient>& clients);
		FileInfo(const std::vector<Azure::Storage::Files::Shares::ShareFileClient>& clients);
		FileInfo(const std::vector<ObjectClient>& clients);
		size_t GetSize() const;
		size_t GetFilePartIndexOfUserOffset(size_t nUserOffset) const;
		std::vector<std::unique_ptr<Azure::Core::IO::BodyStream>>& GetBodyStreams();

	private:
		StorageType storageType;
		std::string sHeader;
		size_t nSize;
		std::vector<PartInfo> parts;
		std::vector<std::unique_ptr<Azure::Core::IO::BodyStream>> bodyStreams;
	};

	struct FilePartInfo
	{
		std::string sHeader;
		size_t nSize;
	};

	struct PartInfo
	{
		size_t nRealOffset;
		size_t nUserOffset;
		size_t nContentSize;
	};
}
