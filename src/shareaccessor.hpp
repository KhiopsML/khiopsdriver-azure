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
	using namespace std;

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

		virtual string GetShareName() const = 0;
		virtual vector<string> GetPath() const = 0;
		virtual string GetServiceUrl() const = 0;
		virtual string GetShareUrl() const = 0;
		virtual Azure::Storage::Files::Shares::ShareServiceClient GetServiceClient() const = 0;
		virtual Azure::Storage::Files::Shares::ShareClient GetShareClient() const = 0;
		virtual Azure::Storage::Files::Shares::ShareDirectoryClient GetDirClient() const = 0;
		virtual Azure::Storage::Files::Shares::ShareFileClient GetFileClient() const = 0;
		
		vector<Azure::Storage::Files::Shares::Models::DirectoryItem> ListDirs() const;
		vector<Azure::Storage::Files::Shares::Models::FileItem> ListFiles() const;
	};
}
