#include "fileaccessor.hpp"
#include "util/string.hpp"

namespace az
{
	const Azure::Core::Url& FileAccessor::GetUrl() const
	{
		return url;
	}

	bool FileAccessor::HasDirUrl() const
	{
		return bHasDirUrl;
	}

	FileAccessor::FileAccessor(const Azure::Core::Url& url):
		url(url),
		bHasDirUrl(EndsWith(url.GetPath(), "/"))
	{
	}
}
