#pragma once

namespace az
{
	class EmulatedFileAccessor;
}

#include <string>
#include <azure/core/url.hpp>
#include <azure/storage/common/storage_credential.hpp>
#include "util/connstring.hpp"

namespace az
{
	using namespace std;

	class EmulatedFileAccessor
	{
	public:
		virtual ~EmulatedFileAccessor() = 0;

	protected:
		EmulatedFileAccessor();

		const ConnectionString& GetConnectionString() const;
		bool IsConnectionStringCompatibleWithUrl(const Azure::Core::Url& url) const;
		shared_ptr<Azure::Storage::StorageSharedKeyCredential> GetCredential() const;

	private:
		ConnectionString connectionString;
		shared_ptr<Azure::Storage::StorageSharedKeyCredential> credential;

		static ConnectionString GetConnectionStringFromEnv();
		shared_ptr<Azure::Storage::StorageSharedKeyCredential> BuildCredential();
	};
}
