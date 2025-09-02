#pragma once

namespace az
{
	class NoFilePartInfoError;
	class FileInfoBase;
	class BlobFileInfo;
	class ShareFileInfo;
}

#include <string>
#include <cstddef>
#include <vector>
#include <memory>
#include <azure/storage/blobs/blob_client.hpp>
#include <azure/storage/files/shares/share_file_client.hpp>
#include <azure/core/io/body_stream.hpp>
#include "exception.hpp"

namespace az
{
	class NoFilePartInfoError : public Error
	{
	public:
		virtual const char* what() const noexcept override;
	};

	class FileInfoBase
	{
	public:
		size_t GetSize() const;

		size_t GetFilePartIndexOfUserOffset(size_t nUserOffset) const;

		std::vector<std::unique_ptr<Azure::Core::IO::BodyStream>>& GetBodyStreams();

	protected:
		struct FilePartInfo;
		struct PartInfo;

		FileInfoBase();

		std::string sHeader;
		size_t nSize;
		std::vector<PartInfo> parts;
		std::vector<std::unique_ptr<Azure::Core::IO::BodyStream>> bodyStreams;

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

		static std::string ReadHeaderFromBodyStream(std::unique_ptr<Azure::Core::IO::BodyStream>& bodyStream);
		static std::string GetFileHeader(const std::vector<FilePartInfo>& filePartInfo);
		static std::vector<PartInfo> GetFileParts(const std::vector<FilePartInfo>& filePartInfo, size_t nHeaderLen);
	};

	class BlobFileInfo : public FileInfoBase
	{
	public:
		BlobFileInfo();

		BlobFileInfo(const std::vector<Azure::Storage::Blobs::BlobClient>& clients);

	private:
		static std::unique_ptr<Azure::Core::IO::BodyStream> DownloadFilePart(const Azure::Storage::Blobs::BlobClient& client, size_t nOffset);
	};

	class ShareFileInfo : public FileInfoBase
	{
	public:
		ShareFileInfo();

		ShareFileInfo(const std::vector<Azure::Storage::Files::Shares::ShareFileClient>& clients);

	private:
		static std::unique_ptr<Azure::Core::IO::BodyStream> DownloadFilePart(const Azure::Storage::Files::Shares::ShareFileClient& client, size_t nOffset);
	};
}
