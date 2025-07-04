#include "cloudshareaccessor.hpp"
#include <regex>
#include "exception.hpp"
#include "util/string.hpp"

namespace az
{
	CloudShareAccessor::CloudShareAccessor(const Azure::Core::Url& url):
		ShareAccessor(url),
		CloudFileAccessor()
	{
	}

	CloudShareAccessor::~CloudShareAccessor()
	{
	}

	Azure::Storage::Files::Shares::ShareFileClient CloudShareAccessor::GetShareFileClient() const
	{
		return Azure::Storage::Files::Shares::ShareFileClient(GetUrl().GetAbsoluteUrl(), GetCredential());
	}

	vector<string> CloudShareAccessor::UrlPathParts() const
	{
		return Split(GetUrl().GetPath(), '/');
	}

	void CloudShareAccessor::CheckFileUrl() const
	{
		const string& path = GetUrl().GetPath();
		smatch match;
		if (!regex_match(path, regex("[^/]+(?:/[^/]+)+")))
		{
			throw InvalidFileUrlPathError(path);
		}
	}

	void CloudShareAccessor::CheckDirUrl() const
	{
		const string& path = GetUrl().GetPath();
		smatch match;
		if (!regex_match(path, regex("(?:[^/]+/){2,}")))
		{
			throw InvalidDirUrlPathError(path);
		}
	}
}
