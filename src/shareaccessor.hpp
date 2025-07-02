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

		bool Exists() const;
		size_t GetSize() const;
		FileStream Open(char mode) const;
		void Remove() const;
		void MkDir() const;
		void RmDir() const;
		size_t GetFreeDiskSpace() const;
		void CopyTo(const Azure::Core::Url& destUrl) const;
		void CopyFrom(const Azure::Core::Url& sourceUrl) const;

	protected:
		ShareAccessor(const Azure::Core::Url& url);

		virtual Azure::Storage::Files::Shares::ShareFileClient GetShareFileClient() const = 0;
	};
}
