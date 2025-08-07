#include "fileaccessor.hpp"
#include "util/string.hpp"

using namespace std;

namespace az
{
	FileAccessor::~FileAccessor()
	{
	}

	FileAccessor::FileAccessor(const Azure::Core::Url& url, const function<const unique_ptr<FileReader>& (unique_ptr<FileReader>)>& registerReader, const function<const unique_ptr<FileWriter>& (unique_ptr<FileWriter>)>& registerWriter, const function<const unique_ptr<FileAppender>& (unique_ptr<FileAppender>)>& registerAppender):
		url(url),
		bHasDirUrl(EndsWith(url.GetPath(), "/")),
		RegisterReader(registerReader),
		RegisterWriter(registerWriter),
		RegisterAppender(registerAppender)
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
