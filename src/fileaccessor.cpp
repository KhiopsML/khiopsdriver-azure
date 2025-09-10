#include "fileaccessor.hpp"
#include "util/string.hpp"

using namespace std;

namespace az
{
	FileAccessor::~FileAccessor()
	{
	}

	FileAccessor::FileAccessor(const Azure::Core::Url& url, const function<unique_ptr<FileReader>& (unique_ptr<FileReader>&&)>& registerReader, const function<unique_ptr<FileOutputStream>& (unique_ptr<FileOutputStream>&&)>& registerWriter) :
		RegisterReader(registerReader),
		RegisterWriter(registerWriter),
		url(url),
		bHasDirUrl(EndsWith(url.GetPath(), "/"))
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
