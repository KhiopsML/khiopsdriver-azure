#include "emulatedfileaccessor.hpp"

namespace az
{
	EmulatedFileAccessor::EmulatedFileAccessor(const string& sConnectionString):
		sConnectionString(sConnectionString)
	{
	}

	const string& EmulatedFileAccessor::GetConnectionString() const
	{
		return sConnectionString;
	}

	bool EmulatedFileAccessor::IsConnectionStringCompatibleWithUrl(const Azure::Core::Url& url) const
	{
		// TODO Implement
		return false;
	}
}
