#pragma once

namespace az
{
	class FileAccessor;
}

#include <string>
#include <azure/core.hpp>
#include "filestream.h"

namespace az
{
	using namespace std;

	class FileAccessor
	{
	public:
		inline const Azure::Core::Url& GetUrl() const;
		inline bool HasDirUrl() const;
		inline shared_ptr<Azure::Core::Credentials::TokenCredential> GetCredential() const;

		virtual bool Exists() const = 0;
		virtual size_t GetSize() const = 0;
		virtual FileStream Open() const = 0;

	protected:
		FileAccessor(const Azure::Core::Url& url);

	private:
		Azure::Core::Url url;
		bool bHasDirUrl;
		shared_ptr<Azure::Core::Credentials::TokenCredential> credential;

		static shared_ptr<Azure::Core::Credentials::TokenCredential> BuildCredential();
	};
}
