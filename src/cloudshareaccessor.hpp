#pragma once

namespace az
{
	class CloudShareAccessor;
}

#include "fileaccessor.hpp"
#include "shareaccessor.hpp"
#include "cloudfileaccessor.hpp"
#include <azure/core.hpp>
#include <azure/storage/files/shares.hpp>

namespace az
{
	class CloudShareAccessor : public ShareAccessor, public CloudFileAccessor
	{
	public:
		CloudShareAccessor(const Azure::Core::Url& url, const std::function<std::unique_ptr<FileReader>& (std::unique_ptr<FileReader>&&)>& registerReader, const std::function<std::unique_ptr<FileOutputStream>& (std::unique_ptr<FileOutputStream>&&)>& registerWriter);
		~CloudShareAccessor();

	protected:
		std::string GetShareName() const override;
		std::vector<std::string> GetPath() const override;
		std::string GetServiceUrl() const override;
		std::string GetShareUrl() const override;
		Azure::Storage::Files::Shares::ShareServiceClient GetServiceClient() const override;
		Azure::Storage::Files::Shares::ShareClient GetShareClient() const override;
		Azure::Storage::Files::Shares::ShareDirectoryClient GetDirClient() const override;
		Azure::Storage::Files::Shares::ShareFileClient GetFileClient() const override;
		std::vector<std::string> UrlPathParts() const override;
		void CheckFileUrl() const override;
		void CheckDirUrl() const override;
	};
}
