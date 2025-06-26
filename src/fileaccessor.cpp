#include "fileaccessor.h"
#include "util/string.h"

namespace az
{
	inline const Azure::Core::Url& FileAccessor::GetUrl() const
	{
		return url;
	}

	inline bool FileAccessor::HasDirUrl() const
	{
		return bHasDirUrl;
	}

	FileAccessor::FileAccessor(const Azure::Core::Url& url):
		url(url),
		bHasDirUrl(EndsWith(url.GetPath(), "/"))
	{
	}
}
