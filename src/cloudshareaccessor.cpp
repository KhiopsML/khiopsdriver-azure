#include "cloudshareaccessor.hpp"
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
}
