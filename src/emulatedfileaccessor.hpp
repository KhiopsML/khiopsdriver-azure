#pragma once

namespace az
{
	class EmulatedFileAccessor;
}

#include <string>
#include <azure/core/url.hpp>
#include <azure/storage/common/storage_credential.hpp>
#include "util.hpp"

namespace az
{
	class EmulatedFileAccessor
	{
	public:
		virtual ~EmulatedFileAccessor() = 0;

	protected:
		EmulatedFileAccessor();

		const util::connstr::ConnectionString& GetConnectionString() const;
		bool IsConnectionStringCompatibleWithUrl(const Azure::Core::Url& url) const;
		std::shared_ptr<Azure::Storage::StorageSharedKeyCredential> GetCredential() const;

	private:
		util::connstr::ConnectionString connectionString;
		std::shared_ptr<Azure::Storage::StorageSharedKeyCredential> credential;

		static util::connstr::ConnectionString GetConnectionStringFromEnv();
		std::shared_ptr<Azure::Storage::StorageSharedKeyCredential> BuildCredential();
	};
}
