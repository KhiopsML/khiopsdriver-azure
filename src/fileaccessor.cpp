#include "fileaccessor.hpp"
#include "util/string.hpp"

namespace az
{
	FileAccessor::~FileAccessor()
	{
	}

	FileAccessor::FileAccessor(const Azure::Core::Url& url):
		url(url),
		bHasDirUrl(EndsWith(url.GetPath(), "/"))
	{
		if (HasDirUrl())
		{
			CheckDirUrl();
		}
		else
		{
			CheckFileUrl();
		}
	}

	const Azure::Core::Url& FileAccessor::GetUrl() const
	{
		return url;
	}

	bool FileAccessor::HasDirUrl() const
	{
		return bHasDirUrl;
	}
}
