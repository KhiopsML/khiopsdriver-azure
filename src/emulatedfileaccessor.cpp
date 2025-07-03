#include "emulatedfileaccessor.hpp"
#include "util/connstring.hpp"
#include "util/env.hpp"
#include "util/string.hpp"

namespace az
{
	EmulatedFileAccessor::~EmulatedFileAccessor()
	{
	}

	EmulatedFileAccessor::EmulatedFileAccessor():
		connectionString(GetConnectionStringFromEnv()),
		credential(BuildCredential())
	{
	}

	const ConnectionString& EmulatedFileAccessor::GetConnectionString() const
	{
		return connectionString;
	}

	bool EmulatedFileAccessor::IsConnectionStringCompatibleWithUrl(const Azure::Core::Url& url) const
	{
		return StartsWith(url.GetAbsoluteUrl(), GetConnectionString().blobEndpoint.GetAbsoluteUrl());
	}

	shared_ptr<Azure::Storage::StorageSharedKeyCredential> EmulatedFileAccessor::GetCredential() const
	{
		return credential;
	}

	ConnectionString EmulatedFileAccessor::GetConnectionStringFromEnv()
	{
		return ParseConnectionString(GetEnvironmentVariableOrThrow("AZURE_STORAGE_CONNECTION_STRING"));
	}

	shared_ptr<Azure::Storage::StorageSharedKeyCredential> EmulatedFileAccessor::BuildCredential()
	{
		const ConnectionString& connStr = GetConnectionString();
		return make_shared<Azure::Storage::StorageSharedKeyCredential>(connStr.sAccountName, connStr.sAccountKey);
	}
}
