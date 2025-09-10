#include "cloudshareaccessor.hpp"
#include <sstream>
#include <regex>
#include <azure/storage/files/shares/share_options.hpp>
#include "exception.hpp"
#include "util/string.hpp"

using namespace std;

namespace az
{
	CloudShareAccessor::CloudShareAccessor(const Azure::Core::Url& url, const function<unique_ptr<FileReader>& (unique_ptr<FileReader>&&)>& registerReader, const function<unique_ptr<FileOutputStream>& (unique_ptr<FileOutputStream>&&)>& registerWriter) :
		ShareAccessor(url, registerReader, registerWriter),
		CloudFileAccessor()
	{
		CheckUrl();
	}

	CloudShareAccessor::~CloudShareAccessor()
	{
	}

	string CloudShareAccessor::GetShareName() const
	{
		return UrlPathParts().at(0);
	}

	vector<string> CloudShareAccessor::GetPath() const
	{
		vector<string> urlPathParts = UrlPathParts();
		return vector<string>(urlPathParts.begin() + 1, urlPathParts.end());
	}

	string CloudShareAccessor::GetServiceUrl() const
	{
		const auto& url = GetUrl();
		return (ostringstream() << url.GetScheme() << "://" << url.GetHost() << ":" << url.GetPort()).str();
	}

	string CloudShareAccessor::GetShareUrl() const
	{
		return (ostringstream() << GetServiceUrl() << "/" << GetShareName()).str();
	}

	Azure::Storage::Files::Shares::ShareServiceClient CloudShareAccessor::GetServiceClient() const
	{
		return Azure::Storage::Files::Shares::ShareServiceClient(GetServiceUrl(), GetCredential());
	}

	Azure::Storage::Files::Shares::ShareClient CloudShareAccessor::GetShareClient() const
	{
		Azure::Storage::Files::Shares::ShareClientOptions opts;
		opts.ShareTokenIntent = Azure::Storage::Files::Shares::Models::ShareTokenIntent::Backup;
		return Azure::Storage::Files::Shares::ShareClient(GetShareUrl(), GetCredential(), opts);
	}

	Azure::Storage::Files::Shares::ShareDirectoryClient CloudShareAccessor::GetDirClient() const
	{
		return GetShareClient().GetRootDirectoryClient();
	}

	Azure::Storage::Files::Shares::ShareFileClient CloudShareAccessor::GetFileClient() const
	{
		Azure::Storage::Files::Shares::ShareClientOptions opts;
		opts.ShareTokenIntent = Azure::Storage::Files::Shares::Models::ShareTokenIntent::Backup;
		return Azure::Storage::Files::Shares::ShareFileClient(GetUrl().GetAbsoluteUrl(), GetCredential(), opts);
	}

	vector<string> CloudShareAccessor::UrlPathParts() const
	{
		return Split(GetUrl().GetPath(), '/', -1, true);
	}

	void CloudShareAccessor::CheckFileUrl() const
	{
		const string& path = GetUrl().GetPath();
		if (!regex_match(path, regex("[^/]+(?:/[^/]+)+")))
		{
			throw InvalidFileUrlPathError(path);
		}
	}

	void CloudShareAccessor::CheckDirUrl() const
	{
		const string& path = GetUrl().GetPath();
		if (!regex_match(path, regex("(?:[^/]+/){2,}")))
		{
			throw InvalidDirUrlPathError(path);
		}
	}
}
