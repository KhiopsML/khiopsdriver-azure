#include "emulatedfileaccessor.hpp"
#include "util.hpp"

using namespace std;

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

	const util::connstr::ConnectionString& EmulatedFileAccessor::GetConnectionString() const
	{
		return connectionString;
	}

	bool EmulatedFileAccessor::IsConnectionStringCompatibleWithUrl(const Azure::Core::Url& url) const
	{
		return util::str::StartsWith(url.GetAbsoluteUrl(), GetConnectionString().blobEndpoint.GetAbsoluteUrl());
	}

	shared_ptr<Azure::Storage::StorageSharedKeyCredential> EmulatedFileAccessor::GetCredential() const
	{
		return credential;
	}

	util::connstr::ConnectionString EmulatedFileAccessor::GetConnectionStringFromEnv()
	{
		return util::connstr::ConnectionString::ParseConnectionString(util::env::GetEnvironmentVariableOrThrow("AZURE_STORAGE_CONNECTION_STRING"));
	}

	shared_ptr<Azure::Storage::StorageSharedKeyCredential> EmulatedFileAccessor::BuildCredential()
	{
		const util::connstr::ConnectionString& connStr = GetConnectionString();
		return make_shared<Azure::Storage::StorageSharedKeyCredential>(connStr.sAccountName, connStr.sAccountKey);
	}
}
