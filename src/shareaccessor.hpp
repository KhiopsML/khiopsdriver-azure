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

		virtual Azure::Storage::Files::Shares::ShareFileClient GetShareFileClient() const = 0;
	};
}
