#pragma once

namespace az
{
	class FileAccessor;
}

#include <string>
#include <azure/core.hpp>
#include "filestream.hpp"

namespace az
{
	using namespace std;

	class FileAccessor
	{
	public:
		const Azure::Core::Url& GetUrl() const;
		bool HasDirUrl() const;
		shared_ptr<Azure::Core::Credentials::TokenCredential> GetCredential() const;

		virtual bool Exists() const = 0;
		virtual size_t GetSize() const = 0;
		virtual FileStream Open(char mode) const = 0;
		virtual void Remove() const = 0;
		virtual void MkDir() const = 0;
		virtual void RmDir() const = 0;
		virtual size_t GetFreeDiskSpace() const = 0;
		virtual void CopyTo(const Azure::Core::Url& destUrl) const = 0;
		virtual void CopyFrom(const Azure::Core::Url& sourceUrl) const = 0;

	protected:
		FileAccessor(const Azure::Core::Url& url);

	private:
		Azure::Core::Url url;
		bool bHasDirUrl;
		shared_ptr<Azure::Core::Credentials::TokenCredential> credential;

		static shared_ptr<Azure::Core::Credentials::TokenCredential> BuildCredential();
	};
}
