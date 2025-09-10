#pragma once

namespace az
{
	class EmulatedBlobAccessor;
}

#include "fileaccessor.hpp"
#include "blobaccessor.hpp"
#include "emulatedfileaccessor.hpp"
#include <azure/core.hpp>
#include <azure/storage/blobs.hpp>

namespace az
{
	class EmulatedBlobAccessor : public BlobAccessor, public EmulatedFileAccessor
	{
	public:
		EmulatedBlobAccessor(const Azure::Core::Url& url, const std::function<std::unique_ptr<FileReader>& (std::unique_ptr<FileReader>&&)>& registerReader, const std::function<std::unique_ptr<FileOutputStream>& (std::unique_ptr<FileOutputStream>&&)>& registerWriter);
		~EmulatedBlobAccessor();

	protected:
		std::string GetAccountName() const;
		std::string GetContainerName() const override;
		std::string GetObjectName() const override;
		std::string GetServiceUrl() const override;
		std::string GetContainerUrl() const override;
		Azure::Storage::Blobs::BlobServiceClient GetServiceClient() const override;
		Azure::Storage::Blobs::BlobContainerClient GetContainerClient() const override;
		Azure::Storage::Blobs::BlobClient GetBlobClient() const override;
		std::vector<std::string> UrlPathParts() const override;
		void CheckFileUrl() const override;
		void CheckDirUrl() const override;
	};
}
