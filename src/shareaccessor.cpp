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

	}

	size_t ShareAccessor::GetSize() const
	{

	}

	FileStream ShareAccessor::Open() const
	{

	}
}
