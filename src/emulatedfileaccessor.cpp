#include "emulatedfileaccessor.hpp"

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
		// TODO Implement
		return false;
	}

	ConnectionString EmulatedFileAccessor::GetConnectionStringFromEnv()
	{
		return ConnectionString();
	}
}
