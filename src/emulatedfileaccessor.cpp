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
		connectionString(GetConnectionStringFromEnv())
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

	ConnectionString EmulatedFileAccessor::GetConnectionStringFromEnv()
	{
		return ParseConnectionString(GetEnvironmentVariableOrThrow("AZURE_STORAGE_CONNECTION_STRING"));
	}
}
