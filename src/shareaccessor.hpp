#pragma once

namespace az
{
	class ShareAccessor;
}

#include "fileaccessor.hpp"
#include <azure/core.hpp>
#include <azure/storage/files/shares.hpp>

namespace az
{
	class ShareAccessor : public FileAccessor
	{
	public:
		virtual ~ShareAccessor() = 0;

		bool Exists() const override;
		size_t GetSize() const override;
		FileStream Open(char mode) const override;
		void Remove() const override;
		void MkDir() const override;
		void RmDir() const override;
		size_t GetFreeDiskSpace() const override;
		void CopyTo(const Azure::Core::Url& destUrl) const override;
		void CopyFrom(const Azure::Core::Url& sourceUrl) const override;

	protected:
		ShareAccessor(const Azure::Core::Url& url);

		virtual std::string GetShareName() const = 0;
		virtual std::vector<std::string> GetPath() const = 0;
		virtual std::string GetServiceUrl() const = 0;
		virtual std::string GetShareUrl() const = 0;
		virtual Azure::Storage::Files::Shares::ShareServiceClient GetServiceClient() const = 0;
		virtual Azure::Storage::Files::Shares::ShareClient GetShareClient() const = 0;
		virtual Azure::Storage::Files::Shares::ShareDirectoryClient GetDirClient() const = 0;
		virtual Azure::Storage::Files::Shares::ShareFileClient GetFileClient() const = 0;
		
		std::vector<Azure::Storage::Files::Shares::ShareDirectoryClient> ListDirs() const;
		std::vector<Azure::Storage::Files::Shares::ShareFileClient> ListFiles() const;
	};
}
