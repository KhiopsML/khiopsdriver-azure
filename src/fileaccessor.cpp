#include "fileaccessor.hpp"
#include "util.hpp"

using namespace std;

namespace az
{
	FileAccessor::~FileAccessor()
	{
	}

	FileAccessor::FileAccessor(const Azure::Core::Url& url, const function<FileReader& (FileReader&&)>& registerReader, const function<FileOutputStream& (FileOutputStream&&)>& registerWriter) :
		RegisterReader(registerReader),
		RegisterWriter(registerWriter),
		url(url),
		bHasDirUrl(util::str::EndsWith(url.GetPath(), "/"))
	{
	}

	const Azure::Core::Url& FileAccessor::GetUrl() const
	{
		return url;
	}

	bool FileAccessor::HasDirUrl() const
	{
		return bHasDirUrl;
	}

	void FileAccessor::CheckUrl() const
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
}
