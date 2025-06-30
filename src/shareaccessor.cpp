#include "shareaccessor.h"
#include "exception.h"

namespace az
{
	ShareAccessor::ShareAccessor(const Azure::Core::Url& url) :
		FileAccessor(url)
	{
	}

	bool ShareAccessor::Exists() const
	{
		// TODO: Implement
		return false;
	}

	size_t ShareAccessor::GetSize() const
	{
		// TODO: Implement
		return 0;
	}

	FileStream ShareAccessor::Open(char mode) const
	{
		// TODO: Implement
		return FileStream();
	}

	void ShareAccessor::Remove() const
	{
		// TODO: Implement
	}

	void ShareAccessor::MkDir() const
	{
		// TODO: Implement
	}

	void ShareAccessor::RmDir() const
	{
		// TODO: Implement
	}

	size_t ShareAccessor::GetFreeDiskSpace() const
	{
		// TODO: Implement
		return 0;
	}

	void ShareAccessor::CopyTo(const Azure::Core::Url& destUrl) const
	{
		// TODO: Implement
	}

	void ShareAccessor::CopyFrom(const Azure::Core::Url& sourceUrl) const
	{
		// TODO: Implement
	}
}
