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
		CloudShareAccessor(const Azure::Core::Url& url);
		~CloudShareAccessor();

	protected:
		string GetShareName() const override;
		vector<string> GetPath() const override;
		string GetServiceUrl() const override;
		string GetShareUrl() const override;
		Azure::Storage::Files::Shares::ShareServiceClient GetServiceClient() const override;
		Azure::Storage::Files::Shares::ShareClient GetShareClient() const override;
		Azure::Storage::Files::Shares::ShareDirectoryClient GetDirClient() const override;
		Azure::Storage::Files::Shares::ShareFileClient GetFileClient() const override;
		vector<string> UrlPathParts() const override;
		void CheckFileUrl() const override;
		void CheckDirUrl() const override;
	};
}
